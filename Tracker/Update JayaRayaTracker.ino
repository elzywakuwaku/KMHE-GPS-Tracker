#include <SPI.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// JAYARAYA TRACKER
// GPS + SUHU + LCD + LORA 915 MHz
// ==========================================

// ---------- DS18B20 ----------
#define DS18B20_PIN 4

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// ---------- LCD 16x2 ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- GPS ----------
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

#define GPS_RX 16
#define GPS_TX 17

// ---------- LoRa ----------
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

#define LORA_BAND 915E6

// ---------- DATA ----------
float suhu = 0.0;
float latitude = 0.0;
float longitude = 0.0;
float kecepatan = 0.0;
float altitude = 0.0;

int satelit = 0;
int gpsFix = 0;

unsigned long waktuKirim = 0;
unsigned long waktuLCD = 0;

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("       JAYARAYA TRACKER");
  Serial.println(" GPS + SUHU + LCD + LORA");
  Serial.println("================================");

  // ---------- LCD ----------
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("JAYARAYA");

  lcd.setCursor(0, 1);
  lcd.print("TRACKER START");

  delay(2000);

  // ---------- DS18B20 ----------
  sensors.begin();

  Serial.println("DS18B20 SIAP");

  // ---------- GPS ----------
  gpsSerial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );

  Serial.println("GPS SIAP");

  // ---------- LoRa ----------
  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  if (!LoRa.begin(LORA_BAND)) {

    Serial.println("LORA GAGAL!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LORA GAGAL!");

    while (true) {
      delay(1000);
    }
  }

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  LoRa.setCodingRate4(5);

  LoRa.enableCrc();

  Serial.println("LORA SIAP");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("JAYARAYA");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  delay(1500);
}

// ==========================================
// LOOP
// ==========================================

void loop() {

  // ---------- BACA GPS ----------
  while (gpsSerial.available()) {

    gps.encode(
      gpsSerial.read()
    );
  }

  // ---------- BACA SUHU ----------
  sensors.requestTemperatures();

  float bacaSuhu =
    sensors.getTempCByIndex(0);

  if (
    bacaSuhu !=
    DEVICE_DISCONNECTED_C
  ) {

    suhu = bacaSuhu;
  }

  // ---------- DATA GPS ----------
  if (
    gps.location.isValid()
  ) {

    gpsFix = 1;

    latitude =
      gps.location.lat();

    longitude =
      gps.location.lng();

    kecepatan =
      gps.speed.kmph();

    altitude =
      gps.altitude.meters();

  }
  else {

    gpsFix = 0;
  }

  if (
    gps.satellites.isValid()
  ) {

    satelit =
      gps.satellites.value();
  }

  // ---------- LCD ----------
  if (
    millis() -
    waktuLCD >= 1000
  ) {

    waktuLCD =
      millis();

    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print("JAYARAYA TRACK");

    lcd.setCursor(0, 1);

    lcd.print("S:");

    lcd.print(
      suhu,
      1
    );

    lcd.print((char)223);

    lcd.print("C ");

    lcd.print("G:");

    if (gpsFix) {

      lcd.print("OK");

    }
    else {

      lcd.print("--");
    }
  }

  // ---------- KIRIM LORA ----------
  if (
    millis() -
    waktuKirim >= 2000
  ) {

    waktuKirim =
      millis();

    String dataKirim =

      String(
        latitude,
        6
      )

      + ","

      + String(
        longitude,
        6
      )

      + ","

      + String(
        kecepatan,
        2
      )

      + ","

      + String(
        altitude,
        1
      )

      + ","

      + String(
        suhu,
        2
      )

      + ","

      + String(
        satelit
      )

      + ","

      + String(
        gpsFix
      );

    LoRa.beginPacket();

    LoRa.print(
      dataKirim
    );

    LoRa.endPacket();

    // ---------- SERIAL ----------
    Serial.println();

    Serial.println(
      "DATA LORA TERKIRIM"
    );

    Serial.print(
      "Suhu      : "
    );

    Serial.print(
      suhu,
      2
    );

    Serial.println(
      " C"
    );

    Serial.print(
      "Latitude  : "
    );

    Serial.println(
      latitude,
      6
    );

    Serial.print(
      "Longitude : "
    );

    Serial.println(
      longitude,
      6
    );

    Serial.print(
      "Speed     : "
    );

    Serial.print(
      kecepatan,
      2
    );

    Serial.println(
      " km/h"
    );

    Serial.print(
      "Satellite : "
    );

    Serial.println(
      satelit
    );

    Serial.print(
      "GPS FIX   : "
    );

    if (gpsFix) {

      Serial.println(
        "YA"
      );

    }
    else {

      Serial.println(
        "BELUM"
      );
    }

    Serial.println(
      "----------------------"
    );
  }
}
