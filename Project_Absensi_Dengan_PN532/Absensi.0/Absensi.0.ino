// ======================================================================
// Sistem Absensi NFC ESP8266
// PN532 + LCD I2C + Buzzer + WiFi + HTTP
// ======================================================================

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Adafruit_PN532.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

unsigned long lastModeCheck = 0;

// ================= WIFI =================
const char* ssid = "Galaxy A14 5G 6090";
const char* password = "5eacfrnjutumpus";

// ================= SERVER ===============
String serverURL = "http://10.223.237.78:5000/scan";

// ================= MODE =================
String mode = "absen";
String modeURL = "http://10.223.237.78:5000/mode";

// ================= PIN ==================
#define LCD_SDA 14
#define LCD_SCL 12

#define PN532_SDA 4
#define PN532_SCL 5

#define BUZZER_PIN 13

#define PN532_IRQ 0
#define PN532_RESET 2

// ================= OBJECT ===============
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
hd44780_I2Cexp lcd;


// =======================================================
// SWITCH I2C
// =======================================================

void useLCD() {
  Wire.begin(LCD_SDA, LCD_SCL);
}

void useNFC() {
  Wire.begin(PN532_SDA, PN532_SCL);
}


// =======================================================
// BUZZER
// =======================================================

void beepSuccess() {
  tone(BUZZER_PIN, 1400, 120);
  delay(150);

  tone(BUZZER_PIN, 2000, 100);
  delay(130);

  tone(BUZZER_PIN, 2500, 250);
  delay(300);

  noTone(BUZZER_PIN);
}


// =======================================================
// LCD
// =======================================================

void lcdShow(String line1, String line2) {
  useLCD();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}


// =======================================================
// WIFI
// =======================================================

bool connectWiFi() {
  lcdShow("Menghubungkan", "ke WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcdShow("WiFi Terkoneksi", WiFi.localIP().toString());

    Serial.print("IP : ");
    Serial.println(WiFi.localIP());

    beepSuccess();
    delay(2500);

    return true;
  }

  return false;
}


// =======================================================
// LCD INIT
// =======================================================

void initLCD() {
  useLCD();

  int status = lcd.begin(16, 2);

  if (status) {
    Serial.println("LCD ERROR!");
    while (true);
  }

  lcd.backlight();

  lcdShow("Inisialisasi...", "Tunggu sebentar");

  delay(1500);
}


// =======================================================
// NFC INIT
// =======================================================

void initNFC() {
  lcdShow("Menyiapkan NFC", "Harap Tunggu");

  useNFC();

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();

  if (!versiondata) {
    Serial.println("PN532 tidak terdeteksi!");
    lcdShow("ERROR NFC", "PN532 gagal");

    while (true);
  }

  Serial.print("PN532 Firmware v");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print(".");
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();

  delay(1000);
}


// =======================================================
// READY SCREEN
// =======================================================

void showReadyScreen() {
  if (mode == "register") {
    lcdShow("Tap kartu anda", "untuk registrasi");
  } else {
    lcdShow("Tap kartu anda", "untuk absensi");
  }
}


// =======================================================
// KIRIM UID KE SERVER
// =======================================================

void kirimUID(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi tidak terhubung");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String url = serverURL + "?uid=" + uid;

  Serial.println("Mengirim ke server:");
  Serial.println(url);

  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    Serial.println("Respon server:");
    Serial.println(payload);

    lcdShow("Server:", payload);
  } else {
    Serial.println("HTTP Error");
    lcdShow("Server Error", "");
  }

  http.end();
}


// =======================================================
// BACA NFC
// =======================================================

void readNFCCard() {
  uint8_t uid[7];
  uint8_t uidLength;

  useNFC();

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    50
  );

  if (!success)
    return;

  String uidString = "";

  Serial.print("UID Kartu: ");

  for (uint8_t i = 0; i < uidLength; i++) {
    Serial.print(uid[i], HEX);
    Serial.print(" ");

    if (uid[i] < 0x10) uidString += "0";
    uidString += String(uid[i], HEX);
  }

  Serial.println();

  uidString.toUpperCase();

  lcdShow("ID Kartu:", uidString);

  kirimUID(uidString);

  beepSuccess();

  delay(2000);

  showReadyScreen();
}


// =======================================================
// CEK MODE
// =======================================================

void cekMode() {
  if (WiFi.status() != WL_CONNECTED)
    return;

  WiFiClient client;
  HTTPClient http;

  http.begin(client, modeURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();

    Serial.print("Mode server: ");
    Serial.println(payload);

    if (payload == "register") {
      if (mode != "register") {
        mode = "register";
        lcdShow("Tap kartu anda", "untuk registrasi");
      }
    } else if (payload == "absen") {
      if (mode != "absen") {
        mode = "absen";
        lcdShow("Tap kartu anda", "untuk absensi");
      }
    }
  }

  http.end();
}


// =======================================================
// SETUP
// =======================================================

void setup() {

  Serial.begin(115200);

  // PERBAIKAN: inisialisasi I2C agar stabil
  Wire.begin();

  pinMode(BUZZER_PIN, OUTPUT);

  initLCD();

  lcdShow("Perangkat Siap", "");
  beepSuccess();
  delay(1500);

  if (!connectWiFi()) {
    lcdShow("WiFi Gagal", "Cek Router");
    while (true);
  }

  initNFC();

  showReadyScreen();
}


// =======================================================
// LOOP
// =======================================================

void loop() {
  if (millis() - lastModeCheck > 1000) {
    cekMode();
    lastModeCheck = millis();
  }

  readNFCCard();
}