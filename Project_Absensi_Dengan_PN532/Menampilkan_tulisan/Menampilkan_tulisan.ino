#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>  // Class untuk PCF8574 backpack

// Buat instance Wire kedua? Tidak, kita switch pin saja
hd44780_I2Cexp lcd;  // Auto-detect alamat & config

#define LCD_SDA 14  // GPIO14 = D5
#define LCD_SCL 12  // GPIO12 = D6

void setup() {
  Serial.begin(115200);
  delay(100);

  // Switch ke pin LCD dulu
  Wire.begin(LCD_SDA, LCD_SCL);       // Set pin custom untuk LCD
  Wire.setClock(100000);              // Optional: turunkan speed biar stabil di ESP8266

  int status = lcd.begin(16, 2);      // Inisialisasi 16x2
  if (status) {                       // !=0 berarti error
    Serial.print("LCD error: ");
    Serial.println(status);
    // Jalankan diagnostic kalau error: Examples > hd44780 > I2CexpDiag
    while (1) delay(1000);
  }

  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LCD Test Sukses!");
  lcd.setCursor(0, 1);
  lcd.print("Pin SDA:D5 SCL:D6");

  Serial.println("LCD OK di pin custom D5 & D6");
}

void loop() {
  // Contoh animasi sederhana
  static int counter = 0;
  lcd.setCursor(0, 1);
  lcd.print("Counter: ");
  lcd.print(counter++);
  lcd.print("   ");  // Clear sisa
  delay(1000);
}