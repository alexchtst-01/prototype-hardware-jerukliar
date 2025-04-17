#include <WiFi.h>
#include <PubSubClient.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// WiFi credentials
const char* ssid = "999";             // your WiFi SSID
const char* password = "11111111";    // your WiFi password

// MQTT broker settings
const char* mqtt_server = "broker.emqx.io";  // replace with your MQTTX broker IP
const int mqtt_port = 1883;                   // usually 1883
const char* mqtt_topic = "jerukliar/send_id_rfid/";

WiFiClient espClient;
PubSubClient client(espClient);

// RFID setup
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
}

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
  connectToWiFi();
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
