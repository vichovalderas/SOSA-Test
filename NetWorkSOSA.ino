#include <WiFi.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <ESPmDNS.h>


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
bool wsStarted = false;

// Red WiFi

const char* ssid = "X";//WIFI NAME
const char* password = "X";//WIFI PASS

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 3000;
int failedAttempts = 0;

// MPU y cuaternión
MPU6050 mpu;
Quaternion q;
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint8_t fifoBuffer[64];


// Datos
Ticker readTicker;
char quatBuffer[128];
volatile bool newQuatReady = false;

void startWebSocketServer() {
  if (!wsStarted) {
    server.addHandler(&ws);
    server.begin();
    wsStarted = true;
    Serial.println("Servidor WebSocket iniciado.");
  }
}


void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Intentando reconectar...");

    WiFi.disconnect(true);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
      delay(200);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconectado. IP: " + WiFi.localIP().toString());
      wsStarted = false;  // reinicia el WebSocket en la siguiente vuelta
    } else {
      Serial.println("\nNo se pudo reconectar.");
    }
  }
}


void readMPUQuat() {
  if (!dmpReady) return;

  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);

    snprintf(quatBuffer, sizeof(quatBuffer),
             "{\"qx\":%.4f,\"qy\":%.4f,\"qz\":%.4f,\"qw\":%.4f}",
             q.x, q.y, q.z, q.w);

    newQuatReady = true;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 20); //INIT PIN I2C

  // Inicializar MPU6050
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  if (devStatus == 0) { //INIT DMP
    Serial.println("DMP inicializado correctamente.");
    mpu.setDMPEnabled(true);
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    Serial.print("Error DMP: ");
    Serial.println(devStatus);
  }

  // WiFi inicial
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {//INIT WIFI
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi. IP: " + WiFi.localIP().toString());

  // WebSocket
  server.addHandler(&ws);
  server.begin();

  // Ticker de lectura de sensor
  readTicker.attach_ms(100, readMPUQuat);

    WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  if (!MDNS.begin("mi-esp")) {//INIT DNS, se muestra en la red privada como ws://mi-esp.local/ws
    Serial.println("Error iniciando mDNS");
    return;
  }
  Serial.println("mDNS iniciado: mi-esp.local");
}

void loop() {
  // Verificación periódica de Wi-Fi
  if (millis() - lastWiFiCheck > wifiCheckInterval) {
    lastWiFiCheck = millis();
    checkWiFiConnection();
  }

  // Si reconectó WiFi, vuelve a iniciar WebSocket
  if (WiFi.status() == WL_CONNECTED && !wsStarted) {
    startWebSocketServer();
  }

  //ws.cleanupClients();

  if (newQuatReady && ws.count() > 0) { //Si hay clientes conectados, es decir ws.count() > 0, se envia quatBuffer a ws://mi-esp.local/ws
    Serial.println(quatBuffer);
    ws.textAll(quatBuffer);
    newQuatReady = false;
  }

  delay(30);
}

