#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

// =================================================
// PIN LORA MRV RFM95W - ESP32 RECEIVER
// =================================================
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  26

#define LORA_BAND 915E6

// =================================================
// WIFI DIBUAT OLEH ESP32
// =================================================
const char* WIFI_NAME = "WiFi GPS Tracker";
const char* WIFI_PASSWORD = "12345678";

WebServer server(80);

// =================================================
// DATA DARI TRACKER
// Format:
// LAT,LON,SPEED,ALT,TEMP,SAT,FIX
// =================================================
double latitude = 0.0;
double longitude = 0.0;

float speedKmh = 0.0;
float altitude = 0.0;
float temperature = 0.0;

int satellites = 0;
int gpsFix = 0;
int loraRSSI = 0;

unsigned long lastPacket = 0;

// =================================================
// WEB DASHBOARD
// =================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>KMHE GPS Tracker</title>

<link rel="stylesheet"
href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">

<script
src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js">
</script>

<style>

body{
  margin:0;
  font-family:Arial,sans-serif;
  background:#111827;
  color:white;
}

header{
  padding:16px;
  text-align:center;
  background:#0b1220;
}

header h2{
  margin:0;
}

header p{
  margin:6px 0 0;
  color:#aab4c4;
}

#map{
  width:100%;
  height:42vh;
}

#status{
  margin:12px;
  padding:12px;
  border-radius:10px;
  text-align:center;
  background:#475569;
}

.online{
  background:#166534 !important;
}

.offline{
  background:#991b1b !important;
}

.grid{
  display:grid;
  grid-template-columns:repeat(2,1fr);
  gap:10px;
  padding:12px;
}

.card{
  background:#1e293b;
  padding:14px;
  border-radius:12px;
}

.label{
  color:#94a3b8;
  font-size:13px;
}

.value{
  font-size:21px;
  font-weight:bold;
  margin-top:6px;
}

</style>
</head>

<body>

<header>
<h2>KMHE GPS TRACKER</h2>
<p>LoRa GPS & Temperature Monitor</p>
</header>

<div id="map"></div>

<div id="status">
Menunggu data tracker...
</div>

<div class="grid">

<div class="card">
<div class="label">Suhu</div>
<div class="value" id="temp">-- °C</div>
</div>

<div class="card">
<div class="label">Kecepatan</div>
<div class="value" id="speed">-- km/h</div>
</div>

<div class="card">
<div class="label">Satelit</div>
<div class="value" id="sat">--</div>
</div>

<div class="card">
<div class="label">Altitude</div>
<div class="value" id="alt">-- m</div>
</div>

<div class="card">
<div class="label">Sinyal LoRa</div>
<div class="value" id="rssi">-- dBm</div>
</div>

<div class="card">
<div class="label">Status GPS</div>
<div class="value" id="gps">--</div>
</div>

</div>

<script>

let map = L.map("map").setView(
[-6.194,106.880],
16
);

L.tileLayer(
"https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
{
maxZoom:19,
attribution:"OpenStreetMap"
}
).addTo(map);

let marker = L.marker(
[-6.194,106.880]
).addTo(map);

let firstLocation = true;

async function updateData(){

try{

const response = await fetch("/data");
const data = await response.json();

document.getElementById("temp").innerHTML =
data.temp + " °C";

document.getElementById("speed").innerHTML =
data.speed + " km/h";

document.getElementById("sat").innerHTML =
data.sat;

document.getElementById("alt").innerHTML =
data.alt + " m";

document.getElementById("rssi").innerHTML =
data.rssi + " dBm";

document.getElementById("gps").innerHTML =
data.fix ? "GPS FIX" : "NO FIX";

let status =
document.getElementById("status");

if(data.online){

status.className = "online";
status.innerHTML = "TRACKER ONLINE";

}else{

status.className = "offline";
status.innerHTML = "TRACKER OFFLINE";

}

if(data.fix){

let position = [
data.lat,
data.lon
];

marker.setLatLng(position);

if(firstLocation){

map.setView(
position,
17
);

firstLocation = false;

}

}

}catch(error){

console.log(error);

}

}

setInterval(
updateData,
1000
);

updateData();

</script>

</body>
</html>
)rawliteral";

// =================================================
// HALAMAN UTAMA
// =================================================
void handleRoot(){

server.send_P(
200,
"text/html",
INDEX_HTML
);

}

// =================================================
// DATA JSON UNTUK DASHBOARD
// =================================================
void handleData(){

bool online =
(millis() - lastPacket) < 5000;

String json = "{";

json += "\"lat\":";
json += String(latitude,6);

json += ",\"lon\":";
json += String(longitude,6);

json += ",\"speed\":";
json += String(speedKmh,1);

json += ",\"alt\":";
json += String(altitude,1);

json += ",\"temp\":";
json += String(temperature,1);

json += ",\"sat\":";
json += String(satellites);

json += ",\"fix\":";
json += String(gpsFix);

json += ",\"rssi\":";
json += String(loraRSSI);

json += ",\"online\":";
json += online ? "true" : "false";

json += "}";

server.send(
200,
"application/json",
json
);

}

// =================================================
// MEMBACA DATA LORA
// =================================================
void parsePacket(String packet){

double newLat;
double newLon;

float newSpeed;
float newAltitude;
float newTemperature;

int newSat;
int newFix;

int result = sscanf(
packet.c_str(),
"%lf,%lf,%f,%f,%f,%d,%d",
&newLat,
&newLon,
&newSpeed,
&newAltitude,
&newTemperature,
&newSat,
&newFix
);

if(result == 7){

latitude = newLat;
longitude = newLon;

speedKmh = newSpeed;
altitude = newAltitude;
temperature = newTemperature;

satellites = newSat;
gpsFix = newFix;

lastPacket = millis();

Serial.println("DATA VALID");

}else{

Serial.println(
"FORMAT DATA TIDAK SESUAI"
);

}

}

// =================================================
// SETUP
// =================================================
void setup(){

Serial.begin(115200);

delay(1000);

Serial.println();
Serial.println("================================");
Serial.println("KMHE GPS RECEIVER");
Serial.println("LoRa + WiFi Dashboard");
Serial.println("================================");

// LORA

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

if(
!LoRa.begin(LORA_BAND)
){

Serial.println(
"LO RA GAGAL!"
);

while(true){
delay(1000);
}

}

LoRa.setSpreadingFactor(7);
LoRa.setSignalBandwidth(125E3);
LoRa.setCodingRate4(5);

LoRa.enableCrc();

Serial.println(
"LoRa 915 MHz SIAP"
);

// WIFI ACCESS POINT

WiFi.mode(WIFI_AP);

WiFi.softAP(
WIFI_NAME,
WIFI_PASSWORD
);

Serial.println();

Serial.print(
"WiFi: "
);

Serial.println(
WIFI_NAME
);

Serial.print(
"Password: "
);

Serial.println(
WIFI_PASSWORD
);

Serial.print(
"IP: "
);

Serial.println(
WiFi.softAPIP()
);

// WEB SERVER

server.on(
"/",
HTTP_GET,
handleRoot
);

server.on(
"/data",
HTTP_GET,
handleData
);

server.begin();

Serial.println(
"WEB SERVER SIAP"
);

}

// =================================================
// LOOP
// =================================================
void loop(){

server.handleClient();

int packetSize =
LoRa.parsePacket();

if(packetSize){

String packet = "";

while(
LoRa.available()
){

packet +=
(char)LoRa.read();

}

loraRSSI =
LoRa.packetRssi();

Serial.print(
"DATA MASUK: "
);

Serial.println(
packet
);

Serial.print(
"RSSI: "
);

Serial.println(
loraRSSI
);

parsePacket(
packet
);

}

}
