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
 
//WiFi params.
const char* ssid = "[wifi-name]";
const char* password = "[wifi-password]";

//I2C Pinout
int SDA_PIN = 9;
int SCL_PIN = 8;

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

void restartMPU() {
  Serial.println("Reiniciando MPU6050...");
  mpu.reset();  // reset I2C y registros
  delay(100);   // darle tiempo al sensor

  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  if (devStatus == 0) {
    mpu.setDMPEnabled(true);
    packetSize = mpu.dmpGetFIFOPacketSize();
    dmpReady = true;
    Serial.println("MPU6050 reiniciado con éxito.");
  } else {
    dmpReady = false;
    Serial.print("Error al reiniciar DMP. Código: ");
    Serial.println(devStatus);
  }
}


void readMPUQuat() {
  if (!dmpReady){
    Serial.println("Fallo en lectura DMP. Reiniciando...");
    restartMPU();
    return;
  } 

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 no responde. Intentando reiniciar...");
    restartMPU();
    return;
  }

  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) ) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    snprintf(quatBuffer, sizeof(quatBuffer),
             "{\"qx\":%.4f,\"qy\":%.4f,\"qz\":%.4f,\"qw\":%.4f}",
             q.x, q.y, q.z, q.w);
    newQuatReady = true;
  } 
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializar MPU6050
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

if (devStatus == 0) {
    mpu.CalibrateAccel(6);  // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateGyro(6);
    Serial.println("These are the Active offsets: ");
    mpu.PrintActiveOffsets();
    Serial.println(F("Enabling DMP..."));   //Turning ON DMP
    mpu.setDMPEnabled(true);

    /*Enable Arduino interrupt detection*/
    Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
    Serial.println(F(")..."));

    /* Set the DMP Ready flag so the main loop() function knows it is okay to use it */
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize(); //Get expected DMP packet size for later comparison
  }   else {
    Serial.print(F("DMP Initialization failed (code ")); //Print the error code
    Serial.print(devStatus);
    Serial.println(F(")"));
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
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
    Serial.println("WebSocket server iniciado!");


  // Ticker de lectura de sensor
  readTicker.attach_ms(100, readMPUQuat);

  if (!MDNS.begin("mi-esp")) {
    Serial.println("Error iniciando mDNS");
    return;
  }
  Serial.println("mDNS iniciado: mi-esp.local");
    /* Supply your gyro offsets here, scaled for min sensitivity */
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);

}

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

  ws.cleanupClients();

  if (newQuatReady && ws.count() > 0) {
    Serial.println(quatBuffer);
    ws.textAll(quatBuffer);
    newQuatReady = false;
  }
  digitalWrite(LED_BUILTIN, !dmpReady);  // encender si hay error
  delay(40);
}

