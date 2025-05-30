#include <WiFi.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
bool wsStarted = false;

// Red WiFi
const char* ssid = "hotSpot";
const char* password = "redmipass";
/*const char* ssid = "VALDERAS";
const char* password = "10203040";*/

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
      wsStarted = false;  // ← reinicia el WebSocket en la siguiente vuelta
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
  Wire.begin(21, 20);

  // Inicializar MPU6050
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  if (devStatus == 0) {
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi. IP: " + WiFi.localIP().toString());

  // WebSocket
  server.addHandler(&ws);
  server.begin();

  // Ticker de lectura de sensor
  readTicker.attach_ms(100, readMPUQuat);
}

int msg=0;

void loop() {
  //int size = sizeof(quatBuffer);
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

  if (newQuatReady && ws.count() > 0) {
    Serial.println(quatBuffer);
    ws.textAll(quatBuffer);
    newQuatReady = false;
    msg++;
  }else {
    Serial.print("Com terminated. Messages:");
    Serial.println(msg);
    //Serial.println("Bytes: "+size);
    }

  delay(30);
}

