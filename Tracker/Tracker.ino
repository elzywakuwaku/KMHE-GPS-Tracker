#include <SPI.h>
#include <LoRa.h>

#include <TinyGPSPlus.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// KONFIGURASI LORA MRV RFM95W
// =====================================================

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  26

#define LORA_BAND 915E6

// =====================================================
// KONFIGURASI GPS NEO-M8N
// GPS TX -> ESP32 GPIO 16
// GPS RX -> ESP32 GPIO 17
// =====================================================

#define GPS_RX 16
#define GPS_TX 17

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// =====================================================
// KONFIGURASI DS18B20
// DATA/SIGNAL -> GPIO 4
// =====================================================

#define DS18B20_PIN 4

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// =====================================================
// KONFIGURASI LCD I2C
// SDA -> GPIO 21
// SCL -> GPIO 22
// Alamat -> 0x27
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// TIMER
// =====================================================

unsigned long waktuKirim = 0;
unsigned long waktuLCD = 0;

const unsigned long INTERVAL_KIRIM = 1000;
const unsigned long INTERVAL_LCD = 1000;

// =====================================================
// VARIABEL
// =====================================================

float suhu = -127.0;

double latitude = 0.0;
double longitude = 0.0;

float kecepatan = 0.0;
float ketinggian = 0.0;

int jumlahSatelit = 0;

bool gpsFix = false;

// =====================================================
// FUNGSI SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("==================================");
  Serial.println(" KMHE GPS TRACKER");
  Serial.println(" GPS + SUHU + LORA 915 MHz");
  Serial.println("==================================");

  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  gpsSerial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );

  Serial.println("GPS NEO-M8N dimulai...");

  // ---------------------------------------------------
  // DS18B20
  // ---------------------------------------------------

  sensors.begin();

  Serial.println("DS18B20 dimulai...");

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  Wire.begin(21, 22);

  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("KMHE TRACKER");

  lcd.setCursor(0, 1);
  lcd.print("Memulai...");

  delay(2000);

  // ---------------------------------------------------
  // LORA
  // ---------------------------------------------------

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

  Serial.println("Memulai LoRa...");

  if (!LoRa.begin(LORA_BAND)) {

    Serial.println("ERROR: LoRa gagal!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LORA ERROR!");

    while (true) {

      delay(1000);

    }

  }

  LoRa.setTxPower(20);

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  LoRa.setCodingRate4(5);

  LoRa.enableCrc();

  Serial.println("LoRa 915 MHz siap!");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LORA READY");

  lcd.setCursor(0, 1);
  lcd.print("GPS + TEMP");

  delay(1500);

}

// =====================================================
// LOOP UTAMA
// =====================================================

void loop() {

  bacaGPS();

  if (
    millis() - waktuLCD
    >=
    INTERVAL_LCD
  ) {

    waktuLCD = millis();

    bacaSuhu();

    tampilLCD();

    tampilSerial();

  }

  if (
    millis() - waktuKirim
    >=
    INTERVAL_KIRIM
  ) {

    waktuKirim = millis();

    kirimLoRa();

  }

}

// =====================================================
// MEMBACA GPS
// =====================================================

void bacaGPS() {

  while (
    gpsSerial.available() > 0
  ) {

    char dataGPS =
      gpsSerial.read();

    gps.encode(dataGPS);

  }

  jumlahSatelit =
    gps.satellites.isValid()
    ?
    gps.satellites.value()
    :
    0;

  if (
    gps.location.isValid()
    &&
    gps.location.age() < 3000
  ) {

    gpsFix = true;

    latitude =
      gps.location.lat();

    longitude =
      gps.location.lng();

  }
  else {

    gpsFix = false;

  }

  if (
    gps.speed.isValid()
  ) {

    kecepatan =
      gps.speed.kmph();

  }

  if (
    gps.altitude.isValid()
  ) {

    ketinggian =
      gps.altitude.meters();

  }

}

// =====================================================
// MEMBACA SUHU
// =====================================================

void bacaSuhu() {

  sensors.requestTemperatures();

  float hasil =
    sensors.getTempCByIndex(0);

  if (
    hasil != DEVICE_DISCONNECTED_C
  ) {

    suhu = hasil;

  }
  else {

    suhu = -127.0;

    Serial.println(
      "DS18B20 tidak terbaca!"
    );

  }

}

// =====================================================
// MENAMPILKAN SUHU DI LCD
// =====================================================

void tampilLCD() {

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("SUHU: ");

  if (
    suhu > -100
  ) {

    lcd.print(
      suhu,
      1
    );

    lcd.print(
      (char)223
    );

    lcd.print("C");

  }
  else {

    lcd.print(
      "ERROR"
    );

  }

  lcd.setCursor(0, 1);

  if (
    gpsFix
  ) {

    lcd.print(
      "GPS: FIX "
    );

    lcd.print(
      jumlahSatelit
    );

    lcd.print(
      " SAT"
    );

  }
  else {

    lcd.print(
      "GPS: NO FIX"
    );

  }

}

// =====================================================
// MENGIRIM DATA KE LORA
// =====================================================

void kirimLoRa() {

  String data;

  if (
    gpsFix
  ) {

    data =
      String(latitude, 6)
      + ","
      + String(longitude, 6)
      + ","
      + String(kecepatan, 1)
      + ","
      + String(ketinggian, 1)
      + ","
      + String(suhu, 2)
      + ","
      + String(jumlahSatelit)
      + ",1";

  }
  else {

    data =
      "0,0,"
      + String(kecepatan, 1)
      + ",0,"
      + String(suhu, 2)
      + ","
      + String(jumlahSatelit)
      + ",0";

  }

  LoRa.beginPacket();

  LoRa.print(data);

  LoRa.endPacket();

  Serial.println();

  Serial.println(
    "=================================="
  );

  Serial.println(
    "DATA DIKIRIM KE LORA"
  );

  Serial.print(
    "Data : "
  );

  Serial.println(
    data
  );

  Serial.println(
    "=================================="
  );

}

// =====================================================
// MENAMPILKAN DATA DI SERIAL MONITOR
// =====================================================

void tampilSerial() {

  Serial.println();

  Serial.println(
    "----------------------------------"
  );

  if (
    gpsFix
  ) {

    Serial.println(
      "GPS FIX"
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

  }
  else {

    Serial.println(
      "GPS BELUM FIX"
    );

  }

  Serial.print(
    "Speed     : "
  );

  Serial.print(
    kecepatan,
    1
  );

  Serial.println(
    " km/h"
  );

  Serial.print(
    "Altitude  : "
  );

  Serial.print(
    ketinggian,
    1
  );

  Serial.println(
    " m"
  );

  Serial.print(
    "Satelit   : "
  );

  Serial.println(
    jumlahSatelit
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

  Serial.println(
    "----------------------------------"
  );

}
