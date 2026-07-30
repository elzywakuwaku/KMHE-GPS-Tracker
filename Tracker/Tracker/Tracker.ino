#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// PIN LORA MRV RFM95W
// =====================================================

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  26

#define LORA_BAND 915E6

// =====================================================
// WIFI HOTSPOT RECEIVER
// =====================================================

const char* AP_SSID = "KMHE-GPS";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

// =====================================================
// DATA DARI TRACKER
// Format:
// LAT,LON,SPEED,ALT,TEMP,SAT,FIX
// =====================================================

double latitude = 0.0;
double longitude = 0.0;

float kecepatan = 0.0;
float altitude = 0.0;
float suhu = 0.0;

int satelit = 0;
int gpsFix = 0;

int rssi = 0;

unsigned long waktuDataTerakhir = 0;

// =====================================================
// HALAMAN DASHBOARD
// =====================================================

const char HTML_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta
name="viewport"
content="width=device-width, initial-scale=1.0"
>

<title>
KMHE GPS Tracker
</title>

<link
rel="stylesheet"
href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
/>

<script
src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js">
</script>

<style>

* {

box-sizing:
border-box;

}

body {

margin: 0;

font-family:
Arial,
sans-serif;

background:
#111827;

color:
white;

}

header {

padding:
18px;

text-align:
center;

background:
#0f172a;

}

header h1 {

margin:
0;

font-size:
22px;

}

header p {

margin:
6px 0 0;

color:
#94a3b8;

}

#map {

height:
45vh;

width:
100%;

}

.grid {

display:
grid;

grid-template-columns:
repeat(
2,
1fr
);

gap:
10px;

padding:
12px;

}

.card {

background:
#1e293b;

border-radius:
14px;

padding:
15px;

box-shadow:
0 3px 12px
rgba(
0,
0,
0,
0.25
);

}

.label {

font-size:
12px;

color:
#94a3b8;

}

.value {

font-size:
22px;

font-weight:
bold;

margin-top:
6px;

}

.status {

padding:
12px;

margin:
0 12px 15px;

border-radius:
12px;

text-align:
center;

background:
#334155;

}

.online {

background:
#166534;

}

.offline {

background:
#991b1b;

}

</style>

</head>

<body>

<header>

<h1>
KMHE GPS TRACKER
</h1>

<p>
LoRa GPS & Temperature Monitor
</p>

</header>

<div id="map"></div>

<div
id="status"
class="status"
>

Menunggu data tracker...

</div>

<div class="grid">

<div class="card">

<div class="label">
Suhu
</div>

<div
class="value"
id="temp"
>

-- °C

</div>

</div>

<div class="card">

<div class="label">
Kecepatan
</div>

<div
class="value"
id="speed"
>

-- km/h

</div>

</div>

<div class="card">

<div class="label">
Satelit
</div>

<div
class="value"
id="sat"
>

--

</div>

</div>

<div class="card">

<div class="label">
Altitude
</div>

<div
class="value"
id="alt"
>

-- m

</div>

</div>

<div class="card">

<div class="label">
Sinyal LoRa
</div>

<div
class="value"
id="rssi"
>

-- dBm

</div>

</div>

<div class="card">

<div class="label">
GPS
</div>

<div
class="value"
id="gps"
>

--

</div>

</div>

</div>

<script>

let map =
L.map(
"map"
).setView(
[
-6.194,
106.880
],
16
);

L.tileLayer(

"https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",

{

maxZoom:
19,

attribution:
"OpenStreetMap"

}

).addTo(
map
);

let marker =
L.marker(
[
-6.194,
106.880
]
).addTo(
map
);

async function
updateData()
{

try {

const response =
await fetch(
"/data"
);

const data =
await response.json();

document
.getElementById(
"temp"
)
.innerHTML =
data.temp
+
" °C";

document
.getElementById(
"speed"
)
.innerHTML =
data.speed
+
" km/h";

document
.getElementById(
"sat"
)
.innerHTML =
data.sat;

document
.getElementById(
"alt"
)
.innerHTML =
data.alt
+
" m";

document
.getElementById(
"rssi"
)
.innerHTML =
data.rssi
+
" dBm";

document
.getElementById(
"gps"
)
.innerHTML =
data.fix
?
"FIX"
:
"NO FIX";

let status =
document
.getElementById(
"status"
);

if (
data.online
)
{

status
.className =
"status online";

status
.innerHTML =
"TRACKER ONLINE";

}
else
{

status
.className =
"status offline";

status
.innerHTML =
"TRACKER OFFLINE";

}

if (
data.fix
)
{

const posisi =
[
data.lat,
data.lon
];

marker
.setLatLng(
posisi
);

map
.setView(
posisi,
17
);

}

}
catch (
error
)
{

console.log(
error
);

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

// =====================================================
// SETUP
// =====================================================

void setup() {

Serial.begin(
115200
);

delay(
1000
);

Serial.println();

Serial.println(
"================================"
);

Serial.println(
"KMHE RECEIVER"
);

Serial.println(
"LoRa + WiFi Dashboard"
);

Serial.println(
"================================"
);

// -----------------------------------------------------
// LORA
// -----------------------------------------------------

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

if (
!LoRa.begin(
LORA_BAND
)
)
{

Serial.println(
"ERROR: LoRa gagal!"
);

while (
true
)
{

delay(
1000
);

}

}

LoRa.setSpreadingFactor(
7
);

LoRa.setSignalBandwidth(
125E3
);

LoRa.setCodingRate4(
5
);

LoRa.enableCrc();

Serial.println(
"LoRa 915 MHz siap!"
);

// -----------------------------------------------------
// WIFI ACCESS POINT
// -----------------------------------------------------

WiFi.mode(
WIFI_AP
);

WiFi.softAP(
AP_SSID,
AP_PASSWORD
);

Serial.println();

Serial.print(
"WiFi: "
);

Serial.println(
AP_SSID
);

Serial.print(
"Password: "
);

Serial.println(
AP_PASSWORD
);

Serial.print(
"IP Dashboard: "
);

Serial.println(
WiFi.softAPIP()
);

// -----------------------------------------------------
// WEB SERVER
// -----------------------------------------------------

server.on(
"/",
HTTP_GET,
[]()
{

server.send_P(
200,
"text/html",
HTML_PAGE
);

}
);

server.on(
"/data",
HTTP_GET,
[]()
{

bool online =

(
millis()
-
waktuDataTerakhir
)

<

5000;

String json = "{";

json +=
"\"lat\":";

json +=
String(
latitude,
6
);

json +=
",";

json +=
"\"lon\":";

json +=
String(
longitude,
6
);

json +=
",";

json +=
"\"speed\":";

json +=
String(
kecepatan,
1
);

json +=
",";

json +=
"\"alt\":";

json +=
String(
altitude,
1
);

json +=
",";

json +=
"\"temp\":";

json +=
String(
suhu,
2
);

json +=
",";

json +=
"\"sat\":";

json +=
String(
satelit
);

json +=
",";

json +=
"\"fix\":";

json +=
String(
gpsFix
);

json +=
",";

json +=
"\"rssi\":";

json +=
String(
rssi
);

json +=
",";

json +=
"\"online\":";

json +=
online
?
"true"
:
"false";

json +=
"}";

server.send(
200,
"application/json",
json
);

}
);

server.begin();

Serial.println(
"Web server siap!"
);

}

// =====================================================
// LOOP
// =====================================================

void loop() {

server.handleClient();

int packetSize =

LoRa.parsePacket();

if (
packetSize
)
{

String data = "";

while (
LoRa.available()
)
{

data +=
(char)
LoRa.read();

}

rssi =
LoRa.packetRssi();

Serial.print(
"Data masuk: "
);

Serial.println(
data
);

bacaData(
data
);

}

}

// =====================================================
// MEMBACA DATA LORA
// =====================================================

void bacaData(
String data
)
{

double latBaru = 0;

double lonBaru = 0;

float speedBaru = 0;

float altBaru = 0;

float suhuBaru = 0;

int satBaru = 0;

int fixBaru = 0;

int hasil =

sscanf(

data.c_str(),

"%lf,%lf,%f,%f,%f,%d,%d",

&latBaru,

&lonBaru,

&speedBaru,

&altBaru,

&suhuBaru,

&satBaru,

&fixBaru

);

if (
hasil == 7
)
{

latitude =
latBaru;

longitude =
lonBaru;

kecepatan =
speedBaru;

altitude =
altBaru;

suhu =
suhuBaru;

satelit =
satBaru;

gpsFix =
fixBaru;

waktuDataTerakhir =
millis();

Serial.println(
"DATA VALID"
);

}
else
{

Serial.println(
"FORMAT DATA SALAH"
);

}

}
