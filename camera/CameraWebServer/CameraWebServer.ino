#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Camera model selection
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "999";
const char* password = "11111111";

// MQTT broker settings
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "jerukliar/send_id_rfid/";

WiFiClient espClient;
PubSubClient client(espClient);

// Flask server URL
const char* serverUrl = "http://192.168.133.158:5000/upload";

void sendCapturedImage();
void callback(char* topic, byte* payload, unsigned int length);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    while (!client.connected()) {
      Serial.print("Connecting to MQTT...");
      if (client.connect("ESP32CAM_Client")) {
        Serial.println("connected.");
        client.subscribe(mqtt_topic);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" retrying in 2 seconds");
        delay(2000);
      }
    }
  }

  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("MQTT message received:");

  // Convert payload to String
  String jsonString;
  for (unsigned int i = 0; i < length; i++) {
    jsonString += (char)payload[i];
  }
  Serial.println("Payload: " + jsonString);

  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  String uid = doc["uid"];
  Serial.println("UID received: " + uid);

  // Trigger image capture
  sendCapturedImage();
}

void sendCapturedImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/octet-stream");

  int httpResponseCode = http.POST(fb->buf, fb->len);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("Image sent successfully: %s\n", response.c_str());
  } else {
    Serial.printf("Failed to send image. HTTP error: %d\n", httpResponseCode);
  }

  http.end();
  esp_camera_fb_return(fb);
}
