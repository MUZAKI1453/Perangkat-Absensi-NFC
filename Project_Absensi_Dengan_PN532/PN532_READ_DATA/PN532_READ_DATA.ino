#include <Wire.h>
#include <Adafruit_PN532.h>


#define PN532_IRQ   (0)   
#define PN532_RESET (2)   

// Inisialisasi PN532 untuk I2C
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {
  Serial.begin(115200);  
  while (!Serial) delay(10);  

  Serial.println("PN532 NFC/RFID Reader - I2C Mode");
  Serial.println("Scanning for tags...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1) { delay(10); }  
  }

  // Print versi firmware
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set konfigurasi umum
  nfc.SAMConfig();  // Configure board to read RFID tags
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer untuk UID (max 7 bytes)
  uint8_t uidLength;                      // Panjang UID

  // Tunggu tag ISO14443A (Mifare, NFC, dll) sampai 500ms
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 500);

  if (success) {
    Serial.println("\nTag ditemukan!");

    // Tampilkan UID dalam hex
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(" 0x"); 
      Serial.print(uid[i], HEX);
    }
    Serial.println("");


    delay(3000);  
  } else {
    // Tidak ada tag, loop lagi
  }
}