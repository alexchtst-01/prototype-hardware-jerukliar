#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5  // SDA
#define RST_PIN 4  // RST

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();          // Init SPI bus
  rfid.PCD_Init();      // Init MFRC522
  Serial.println("Scan an RFID card...");
}

void loop() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Show UID
  Serial.print("Card UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Halt PICC (stop reading)
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
