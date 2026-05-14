// ╔══════════════════════════════════════════════════════════╗
// ║   ESP32 — IoT Hastane Monitör Sistemi                   ║
// ║   LM35 Sıcaklık + Pulse Sensor + LCD + ThingSpeak      ║
// ║   Öznur Yakut                                           ║
// ╚══════════════════════════════════════════════════════════╝

#include <WiFi.h>
#include "ThingSpeak.h"
#include <LiquidCrystal.h>
#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>

// WiFi Bilgileri
const char* ssid     = "Digiturk_ZTK6H4";     // WiFi adı
const char* password = "UFadTA7R3kFh";  // WiFi şifre

//  ThingSpeak Bilgileri 
unsigned long channelID   = 2475738;         // Channel ID
const char*   writeAPIKey = "BQUOV5LCXFK23KSB";  // Write API

//Pin Tanımlamaları 
#define LM35_PIN   34    // LM35 analog pin
#define PULSE_PIN  35    // Pulse Sensor analog pin
#define LED_PIN     2    // Kalp atışında yanan LED

// LCD bağlantısı: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

// Pulse Sensor 
PulseSensorPlayground pulseSensor;
const int THRESHOLD = 550;

// Değişkenler
WiFiClient client;
int   myBPM   = 0;
float sicaklik = 0.0;

unsigned long previousMillis = 0;
const long    interval       = 15000;  // ThingSpeak min. 15 saniye

//  Setup 
void setup() {
  Serial.begin(115200);

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // LCD başlat
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("  IoT Patient");
  lcd.setCursor(0, 1);
  lcd.print(" Monitor System");
  delay(1500);
  lcd.clear();

  // Pulse Sensor
  pulseSensor.analogInput(PULSE_PIN);
  pulseSensor.blinkOnPulse(LED_PIN);  // LED kalp atışıyla yanıp söner
  pulseSensor.setThreshold(THRESHOLD);
  pulseSensor.begin();

  // WiFi bağlan
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("WiFi baglaniyor");
  Serial.print("WiFi'ye baglaniliyor");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nBaglandi! IP: " + WiFi.localIP().toString());

  // LCD'de IP göster
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Baglandi!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();

  // ThingSpeak başlat
  ThingSpeak.begin(client);
  Serial.println("Sistem hazir!");
}


void loop() {

  // Kalp atışı oku
  myBPM = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) {
    Serial.print("Kalp Atisi: ");
    Serial.print(myBPM);
    Serial.println(" BPM");
  }

  //  LM35 sıcaklık oku 
  // ESP32 ADC: 3.3V, 12-bit (4095)
  // LM35: 10mV/°C
int raw = 0;
for(int i = 0; i < 10; i++) {
  raw += analogRead(LM35_PIN);
  delay(2);
}
raw /= 10;
float voltage = raw * (3.3 / 4095.0);
sicaklik      = voltage * 100.0;

  //LCD güncelle 
  lcd.setCursor(0, 0);
  lcd.print("BODY:");
  lcd.print(sicaklik, 1);
  lcd.print(" *C  ");

  lcd.setCursor(0, 1);
  lcd.print("HEART:");
  lcd.print(myBPM);
  lcd.print(" BPM  ");

  //  ThingSpeak'e gönder (her 15 saniyede bir) 
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    ThingSpeak.setField(1, myBPM);
    ThingSpeak.setField(2, sicaklik);

    int kod = ThingSpeak.writeFields(channelID, writeAPIKey);
    if (kod == 200) {
      Serial.println("ThingSpeak'e gonderildi!");
    } else {
      Serial.print("Hata kodu: ");
      Serial.println(kod);
    }
  }

  delay(20);
}
