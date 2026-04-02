#include <Wire.h>
#include <Adafruit_PN532.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// ── PN532 ────────────────────────────────────────────────
#define PN532_IRQ   (0)     // D3/GPIO0, bisa -1 kalau tidak pakai IRQ
#define PN532_RESET (2)     // D4/GPIO2, bisa -1 kalau tidak pakai RESET
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// ── LCD I2C di pin terpisah ──────────────────────────────
#define LCD_SDA 14          // D5 / GPIO14
#define LCD_SCL 12          // D6 / GPIO12
hd44780_I2Cexp lcd;         // auto-detect alamat I2C LCD

// Pin default PN532 (untuk switch kembali)
#define PN532_SDA 4         // D2 / GPIO4
#define PN532_SCL 5         // D1 / GPIO5

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Absensi NFC + LCD mulai...");

  // ── Inisialisasi PN532 di pin default ──────────────────
  Wire.begin(PN532_SDA, PN532_SCL);
  Wire.setClock(100000);  // lebih stabil di ESP8266

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Tidak menemukan PN532!");
    while (1) delay(1000);
  }
  nfc.SAMConfig();
  Serial.println("PN532 OK");

  // ── Inisialisasi LCD di pin custom ─────────────────────
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  int status = lcd.begin(16, 2);
  if (status) {
    Serial.print("LCD error: ");
    Serial.println(status);
    while (1) delay(1000);
  }

  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap Kartu anda !");
  lcd.setCursor(0, 1);
  lcd.print("                ");  // kosongkan baris 2

  Serial.println("LCD OK - Siap digunakan");
}

void loop() {
  // Pastikan switch ke pin PN532 sebelum baca
  Wire.begin(PN532_SDA, PN532_SCL);

  uint8_t success;
  uint8_t uid[7] = {0};
  uint8_t uidLength;

  // Coba baca tag selama 300 ms (agar responsif)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 300);

  if (success) {
    // Buat string UID HEX (tanpa 0x, uppercase, leading zero)
    String cardUID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) cardUID += "0";
      cardUID += String(uid[i], HEX);
    }
    cardUID.toUpperCase();

    Serial.print("Kartu terdeteksi → UID: ");
    Serial.println(cardUID);

    // Switch ke pin LCD untuk update tampilan
    Wire.begin(LCD_SDA, LCD_SCL);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ID: ");
    lcd.print(cardUID.substring(0, 8));  // tampil 8 karakter pertama

    lcd.setCursor(0, 1);
    lcd.print("Terima kasih !");

    delay(2500);  // tampilkan selama 2.5 detik

    // Kembali ke tampilan standby
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tap Kartu anda !");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
  else {
    // Tidak ada kartu → pastikan tampilan standby tetap
    // (hanya update sekali di awal, jadi kita tidak perlu update terus-menerus)
    // Kalau ingin refresh periodik, bisa uncomment di bawah ini:
    // Wire.begin(LCD_SDA, LCD_SCL);
    // lcd.setCursor(0, 0);
    // lcd.print("Tap Kartu anda !");
    // lcd.setCursor(0, 1);
    // lcd.print("                ");
  }

  delay(100);  // loop ringan agar tidak terlalu cepat
}