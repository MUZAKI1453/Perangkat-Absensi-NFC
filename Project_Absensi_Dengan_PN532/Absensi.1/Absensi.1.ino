// ======================================================================
// FINAL FIX - NFC ABSENSI + BUTTON MODE STABIL (ANTI DOUBLE + ANTI SPAM)
// ======================================================================

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Adafruit_PN532.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


// ================= NTP TIME (WIB) =================
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 10000;  // update jam setiap 10 detik
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);  
// 25200 = UTC+7 (WIB), update setiap 60 detik

// ================= WIFI =================
const char* ssid = "SheUngu"; //"FAMILY USIN WIRA"; //"RAUFA"; // "Galaxy A14 5G 6090"; 
const char* password = "Arkana1901"; //"usinwira"; //"hizam170716"; // "5eacfrnjutumpus";

String serverURL =  "http://192.168.100.127:5000/scan"; //"http://192.168.1.8:5000/scan"; //"http://192.168.1.8:5000/scan"; //  "http://10.206.122.78:5000/scan";

// ================= MODE =================
String mode = "absen";

// ================= PIN ==================
#define LCD_SDA 14
#define LCD_SCL 12

#define PN532_SDA 4
#define PN532_SCL 5

#define BUZZER_PIN 13

#define PN532_IRQ 0
#define PN532_RESET 2

#define BTN_ABSEN   2
#define BTN_DAFTAR  12

// ================= OBJECT ===============
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
hd44780_I2Cexp lcd;

// ================= ANTI DOUBLE =================
String lastUID = "";
unsigned long lastScanTime = 0;
const int delayScan = 3000; // 3 detik

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
  tone(BUZZER_PIN, 1500, 100);
  delay(120);
  tone(BUZZER_PIN, 2000, 120);
  delay(150);
  noTone(BUZZER_PIN);
}

// =======================================================
// LCD
// =======================================================

void lcdShow(String l1, String l2) {
  useLCD();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// =======================================================
// RUNNING TEXT / SCROLLING MESSAGE (Non-blocking)
// =======================================================

void showScrollingMessage(String uid, String message, unsigned long duration = 4000) {
  useLCD();
  lcd.clear();

  // Baris 1: Tampilkan UID
  lcd.setCursor(0, 0);
  lcd.print("Kartu: ");
  lcd.print(uid.substring(0, 9));   // batasi agar tidak overflow

  // Persiapan untuk running text di baris 2
  String fullMsg = "  " + message + "     ";   // tambah spasi di awal & akhir biar mulus
  int len = fullMsg.length();
  unsigned long startTime = millis();
  int pos = 0;

  while (millis() - startTime < duration) {     // tampilkan selama 'duration' milidetik
    handleButton();   // tetap bisa ganti mode

    // Tampilkan 16 karakter yang bergeser
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      int idx = (pos + i) % len;
      lcd.print(fullMsg[idx]);
    }

    pos++;                    // geser posisi
    if (pos > len) pos = 0;

    delay(300);               // kecepatan scrolling (300ms = cukup nyaman)
  }

  // Kembali ke layar Ready setelah selesai scrolling
  showReady();
}

// =======================================================
// WIFI
// =======================================================

bool connectWiFi() {
  lcdShow("Connecting WiFi", "...");

  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 30) {
    delay(500);
    i++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcdShow("WiFi OK", WiFi.localIP().toString());
    beepSuccess();
    delay(1500);
    return true;
  }

  return false;
}

// =======================================================
// INIT LCD
// =======================================================

void initLCD() {
  useLCD();
  lcd.begin(16, 2);
  lcd.backlight();
}

// =======================================================
// INIT NFC
// =======================================================

void initNFC() {
  lcdShow("Init NFC...", "");
  useNFC();

  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    lcdShow("NFC ERROR", "");
    while (true);
  }

  nfc.SAMConfig();
}

// =======================================================
// READY SCREEN
// =======================================================

void showReady() {
  timeClient.update();                    
  
  String jamMenit = timeClient.getFormattedTime().substring(0, 5);  

  String baris1;

  if (mode == "absen") {
    baris1 = "  ABSEN  " + jamMenit;     
  } 
  else {
    baris1 = "REGISTER   " + jamMenit;     
  }

  lcdShow(baris1, "Tap kartu anda !");
}

// =======================================================
// BUTTON HANDLE
// =======================================================

void handleButton() {

  if (digitalRead(BTN_ABSEN) == LOW) {
    delay(50);
    if (digitalRead(BTN_ABSEN) == LOW) {
      mode = "absen";
      lcdShow("Mode:", "ABSENSI");
      beepSuccess();

      while (digitalRead(BTN_ABSEN) == LOW);
      delay(200);

      showReady();
    }
  }

  if (digitalRead(BTN_DAFTAR) == LOW) {
    delay(50);
    if (digitalRead(BTN_DAFTAR) == LOW) {
      mode = "register";
      lcdShow("Mode:", "REGISTER");
      beepSuccess();

      while (digitalRead(BTN_DAFTAR) == LOW);
      delay(200);

      showReady();
    }
  }
}

// =======================================================
// KIRIM DATA
// =======================================================

void kirimUID(String uid) {

  if (WiFi.status() != WL_CONNECTED) {
    lcdShow("WiFi Error", "");
    delay(1500);
    showReady();
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String url = serverURL + "?uid=" + uid + "&mode=" + mode;

  http.begin(client, url);
  int code = http.GET();

  String response = "";
  if (code > 0) {
    response = http.getString();
    response.trim();
  } else {
    response = "Server Error";
  }

  http.end();

  // Tampilkan dengan running text
  showScrollingMessage(uid, response, 5000);   // tampilkan 5 detik
}

// =======================================================
// NFC READ (UPDATED ANTI DOUBLE + SCROLLING)
// =======================================================

void readNFC() {

  uint8_t uid[7];
  uint8_t len;

  useNFC();

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 50))
    return;

  String id = "";

  for (int i = 0; i < len; i++) {
    if (uid[i] < 0x10) id += "0";
    id += String(uid[i], HEX);
  }

  id.toUpperCase();

  // ================= ANTI DOUBLE =================
  if (id == lastUID && (millis() - lastScanTime < delayScan)) {
    return;
  }

  lastUID = id;
  lastScanTime = millis();

  
  lcdShow("Kartu:", id);        // Tampilkan UID sebentar
  delay(800);                   // Beri waktu ±0.8 detik supaya UID terlihat

  kirimUID(id);                 // Di dalam kirimUID sudah ada showScrollingMessage()

  beepSuccess();                // Bunyi sukses setelah kirim
}

// =======================================================
// SETUP
// =======================================================

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BTN_ABSEN, INPUT_PULLUP);
  pinMode(BTN_DAFTAR, INPUT_PULLUP);

  initLCD();

  lcdShow("Starting...", "");
  delay(1000);

  if (!connectWiFi()) {
    lcdShow("WiFi Failed", "");
    while (true);
  }

  timeClient.begin();
  timeClient.update();        // ambil waktu pertama kali

  initNFC();

  showReady();
}

// =======================================================
// LOOP
// =======================================================

void loop() {
  handleButton();

  // Update jam di layar Ready setiap 10 detik
  if (millis() - lastUpdateTime > updateInterval) {
    showReady();
    lastUpdateTime = millis();
  }

  readNFC();
}