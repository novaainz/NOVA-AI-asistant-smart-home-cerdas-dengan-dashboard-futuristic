#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --- KELOMPOK WIFI ---
const char* ssid = "vivo Y27";
const char* password = "novaleonbersama";

WebServer server(80);

// --- PIN SENSOR ---
#define DHTPIN 4       
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int trigPin = 12; 
const int echoPin = 14; 

// --- PIN 6 LED / PERANGKAT ---
const int pinLampu = 23;      // lampu
const int pinTv = 22;         // tv
const int pinKulkas = 21;     // kulkas
const int pinAc = 19;         // ac
const int pinMesinCuci = 18;  // mesin_cuci
const int pinRadio = 5;       // radio

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  int pins[] = {pinLampu, pinTv, pinKulkas, pinAc, pinMesinCuci, pinRadio};
  for(int p : pins) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP ESP32 Kamu: ");
  Serial.println(WiFi.localIP());

  // Dua endpoint utama untuk n8n dan web dashboard:
  server.on("/control", HTTP_POST, handleControl);    
  server.on("/sensor", HTTP_GET, handleSensorData);    

  server.begin();
  Serial.println("ESP32 Web Server Siap!");
}

void loop() {
  server.handleClient();
}

// 1. Handler Kontrol 6 Perangkat
void handleControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  
  String device = doc["device"]; 
  String status = doc["status"]; 
  int state = (status == "on") ? HIGH : LOW;

  if (device == "lampu") digitalWrite(pinLampu, state);
  else if (device == "tv") digitalWrite(pinTv, state);
  else if (device == "kulkas") digitalWrite(pinKulkas, state);
  else if (device == "ac") digitalWrite(pinAc, state);
  else if (device == "mesin_cuci") digitalWrite(pinMesinCuci, state);
  else if (device == "radio") digitalWrite(pinRadio, state);

  Serial.println("Perangkat " + device + " -> " + status);
  
  // Kirim header CORS juga untuk kontrol POST jika diperlukan
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"success\"}");
}

// 2. Handler Ambil Data Sensor & Status Perangkat (DIPERBAIKI URUTAN CORS)
void handleSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;

  StaticJsonDocument<300> doc;
  doc["suhu"] = isnan(t) ? 28.0 : t;
  doc["kelembapan"] = isnan(h) ? 60.0 : h;
  doc["jarak"] = distance;

  // Status ke-6 perangkat
  doc["lampu"] = (digitalRead(pinLampu) == HIGH) ? "on" : "off";
  doc["tv"] = (digitalRead(pinTv) == HIGH) ? "on" : "off";
  doc["kulkas"] = (digitalRead(pinKulkas) == HIGH) ? "on" : "off";
  doc["ac"] = (digitalRead(pinAc) == HIGH) ? "on" : "off";
  doc["mesin_cuci"] = (digitalRead(pinMesinCuci) == HIGH) ? "on" : "off";
  doc["radio"] = (digitalRead(pinRadio) == HIGH) ? "on" : "off";

  String jsonStr;
  serializeJson(doc, jsonStr);

  // HEADER CORS HARUS DI ATAS server.send
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonStr);
}