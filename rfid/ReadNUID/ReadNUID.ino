#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// MQTT broker settings
const char* mqtt_server = "broker.emqx.io";  // replace with your MQTT broker
const int mqtt_port = 1883;
const char* mqtt_topic = "jerukliar/send_id_rfid/";

WiFiClient espClient;
PubSubClient client(espClient);

// RFID setup
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_RFID_Client")) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Setup WiFi using WiFiManager
  WiFiManager wm;
  bool res = wm.autoConnect("ESP32-RFID-Setup", "12345678");

  if (!res) {
    Serial.println("WiFiManager failed to connect. Restarting...");
    ESP.restart();
  }

  Serial.println("WiFi connected using WiFiManager.");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);

  mfrc522.PCD_Init();    
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial); 
  Serial.println(F("Scan card to send UID via MQTT..."));
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Convert UID to HEX string
  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidStr += "0";
    }
    uidStr += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();

  Serial.println("UID: " + uidStr);

  // Build JSON string and publish
  String jsonPayload = "{\"uid\":\"" + uidStr + "\"}";
  client.publish(mqtt_topic, jsonPayload.c_str());
  Serial.println("Published UID to MQTT: " + jsonPayload);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(2000);
}
