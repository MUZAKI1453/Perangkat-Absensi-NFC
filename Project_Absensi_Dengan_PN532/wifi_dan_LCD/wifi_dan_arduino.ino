#include <ESP8266WiFi.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>  // untuk LCD I2C PCF8574 backpack

// ── WiFi credentials ───────────────────────────────────────
const char* ssid     = "NamaWiFiKamu";      // GANTI dengan SSID WiFi kamu
const char* password = "PasswordWiFiKamu";  // GANTI dengan password WiFi

// ── Pin LCD (gunakan pin terpisah dari PN532) ──────────────
#define LCD_SDA 14   // D5 / GPIO14
#define LCD_SCL 12   // D6 / GPIO12

// Buat objek LCD (auto-detect alamat I2C)
hd44780_I2Cexp lcd;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP8266 WiFi + LCD Test ===");

  // ── Inisialisasi LCD ─────────────────────────────────────
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);  // lebih stabil di ESP8266

  int status = lcd.begin(16, 2);
  if (status) {
    Serial.print("LCD gagal inisialisasi! Error: ");
    Serial.println(status);
    while (1) delay(1000);  // halt kalau LCD error
  }

  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Memulai...");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  Serial.println("Menghubungkan ke WiFi...");

  // ── Koneksi WiFi ─────────────────────────────────────────
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 15000;  // 15 detik timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    Serial.print(".");
    
    // Animasi titik di LCD
    lcd.setCursor(12, 1);
    lcd.print("   ");
    lcd.setCursor(12, 1);
    lcd.print(String(millis() / 500 % 4, DEC));  // 0..3 berputar
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung!");
    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Terhubung!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    Serial.println("\nGagal konek WiFi!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Gagal!");
    lcd.setCursor(0, 1);
    lcd.print("Cek SSID/Pass");
  }
}

void loop() {
  // Tampilkan status WiFi setiap 10 detik di LCD & Serial
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 10000) {
    lastUpdate = millis();

    if (WiFi.status() == WL_CONNECTED) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi OK");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());

      Serial.print("Signal: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Putus!");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");

      WiFi.reconnect();  // coba reconnect otomatis
    }
  }

  delay(100);  // loop ringan
}