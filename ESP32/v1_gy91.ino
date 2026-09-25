#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> 
#include "time.h"        

const char* ssid = "M22_C129";
const char* password = "90093656";

AsyncWebServer server(80);
AsyncEventSource events("/events");

#define SDA_PIN    D4
#define SCL_PIN    D5
#define LED_RED    D6  
#define LED_GREEN  D7  
#define BUTTON_PIN D3

MPU9250_asukiaaa mpu;

// Variabel Bias Sensor
float aX_bias = 0, aY_bias = 0, aZ_bias = 0;
float gX_bias = 0, gY_bias = 0, gZ_bias = 0;
float roll_bias = 0, pitch_bias = 0, yaw_bias = 0;

// GABUNGAN: Variabel Kalibrasi Magnetometer 3D murni dari Kode 1
float mX_min = 32000, mX_max = -32000;
float mY_min = 32000, mY_max = -32000;
float mZ_min = 32000, mZ_max = -32000;
float mX_bias = 0, mY_bias = 0, mZ_bias = 0;

unsigned long lastTime = 0;
unsigned long lastSseSend = 0; 
bool lastButton = HIGH;

float wrapAngle(float a) {
  while (a >  180) a -= 360;
  while (a < -180) a += 360;
  return a;
}

String getTimeStamp() {
  return String(millis());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);   
  pinMode(LED_GREEN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.setSleep(false); 
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  events.onConnect([](AsyncEventSourceClient *client){
    client->send("Terhubung ke ESP32", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

  Wire.begin(SDA_PIN, SCL_PIN);
  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  mpu.beginMag();

  // -----------------------------------------------------------------
  // FASE 1: KALIBRASI ACCELEROMETER (DIAMKAN SENSOR)
  // -----------------------------------------------------------------
  Serial.println("\n[1/2] DIAMKAN SENSOR (Kalibrasi Accel)...");
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  delay(1000);

  float sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < 50; i++) {
    mpu.accelUpdate();
    sumX += mpu.accelX() * 9.80665;
    sumY += -mpu.accelZ() * 9.80665;
    sumZ += -mpu.accelY() * 9.80665;
    delay(10);
  }
  aX_bias = sumX / 50.0;
  aY_bias = sumY / 50.0;
  float avgZ = sumZ / 50.0;
  if (avgZ > 0) { aZ_bias = avgZ - 9.80665; } else { aZ_bias = avgZ + 9.80665; }

  // -----------------------------------------------------------------
  // FASE 2: KALIBRASI MAGNETOMETER 3D
  // -----------------------------------------------------------------
  Serial.println("\n[2/2] >>> PUTAR ALAT SEKARANG! 360 DERAJAT DATAR (Waktu 5 Detik) <<<");
  unsigned long timer = millis();
  
  while (millis() - timer < 5000) {
    if ((millis() - timer) % 200 < 100) {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
    } else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }

    if (mpu.magUpdate() == 0) {
      float mx_raw = mpu.magX(); 
      float my_raw = mpu.magY(); 
      float mz_raw = mpu.magZ();
      
      if (mx_raw != 0.0 || my_raw != 0.0 || mz_raw != 0.0) {
        if (mx_raw < mX_min) mX_min = mx_raw; if (mx_raw > mX_max) mX_max = mx_raw;
        if (my_raw < mY_min) mY_min = my_raw; if (my_raw > mY_max) mY_max = my_raw;
        if (mz_raw < mZ_min) mZ_min = mz_raw; if (mz_raw > mZ_max) mZ_max = mz_raw;
      }
    }
    delay(10); 
  }
  
  // Hitung Hard-Iron Offset penuh untuk 3 Sumbu
  mX_bias = (mX_max + mX_min) / 2.0;
  mY_bias = (mY_max + mY_min) / 2.0;
  mZ_bias = (mZ_max + mZ_min) / 2.0; 

  Serial.println("\n>>> BERHENTI MEMUTAR! HADAPKAN KE DEPAN & DIAMKAN <<<");
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(2000);

  // Ambil data awal untuk tare/bias sudut Yaw pertama kali
  mpu.accelUpdate();
  mpu.magUpdate();
  
  float r_ax = mpu.accelX();
  float r_ay = mpu.accelY();
  float r_az = mpu.accelZ();
  
  float rollMag  = atan2(r_ay, r_az);
  float pitchMag = atan2(-r_ax, sqrt(r_ay * r_ay + r_az * r_az));
  
  float mX_bersih = mpu.magX() - mX_bias;
  float mY_bersih = mpu.magY() - mY_bias;
  float mZ_bersih = mpu.magZ() - mZ_bias;

  float Xh = mX_bersih * cos(pitchMag) + mZ_bersih * sin(pitchMag);
  float Yh = mX_bersih * sin(rollMag) * sin(pitchMag) + mY_bersih * cos(rollMag) - mZ_bersih * sin(rollMag) * cos(pitchMag);

  yaw_bias = atan2(Yh, Xh) * 180.0 / PI;
  if (yaw_bias < 0) yaw_bias += 360.0;

  Serial.println("\nSistem Siap! Menunggu gerakan...");
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  lastTime = millis();
}

void loop() {
  unsigned long nowMillis = millis();

  int statusAccel = mpu.accelUpdate();
  int statusGyro  = mpu.gyroUpdate();
  int statusMag   = mpu.magUpdate();

  // Menyimpan data raw accelerometer fisik chip untuk penstabil filter Yaw
  static float raw_chip_ax = 0, raw_chip_ay = 0, raw_chip_az = 1.0;
  if (statusAccel == 0) {
    raw_chip_ax = mpu.accelX();
    raw_chip_ay = mpu.accelY();
    raw_chip_az = mpu.accelZ();
  }

  // GABUNGAN: Kalkulasi Yaw Linier + Kompensasi Kemiringan 3D murni dari Kode 1
  static float raw_yaw = 0; 
  if (statusMag == 0) {
    float mX_bersih = mpu.magX() - mX_bias;
    float mY_bersih = mpu.magY() - mY_bias;
    float mZ_bersih = mpu.magZ() - mZ_bias;

    // Kalkulasi sudut kemiringan fisik asli milik tubuh chip IMU
    float rollRad  = atan2(raw_chip_ay, raw_chip_az);
    float pitchRad = atan2(-raw_chip_ax, sqrt(raw_chip_ay * raw_chip_ay + raw_chip_az * raw_chip_az));

    // Rumus Proyeksi Tilt Compensation 3D Aerospace
    float Xh = mX_bersih * cos(pitchRad) + mZ_bersih * sin(pitchRad);
    float Yh = mX_bersih * sin(rollRad) * sin(pitchRad) + mY_bersih * cos(rollRad) - mZ_bersih * sin(rollRad) * cos(pitchRad);

    raw_yaw = atan2(Yh, Xh) * 180.0 / PI;
    if (raw_yaw < 0) {
      raw_yaw += 360.0;
    }
  }

  // Blok Utama Logika Anda (Tetap murni, tidak ada yang dirusak)
  if (statusAccel == 0 && statusGyro == 0) {

    // Pemetaan sumbu kustom kesukaan Anda tetap aman di sini
    float aX = (mpu.accelX() * 9.80665) - aX_bias;
    float aY = (-mpu.accelZ() * 9.80665) - aY_bias; 
    float aZ = (-mpu.accelY() * 9.80665) - aZ_bias;

    float gX = mpu.gyroX(); 
    float gY = -mpu.gyroZ(); 
    float gZ = mpu.gyroY();

    float raw_roll  = atan2(aX, sqrt(aY * aY + aZ * aZ)) * 180.0 / PI; 
    float raw_pitch = -atan2(aY, sqrt(aX * aX + aZ * aZ)) * 180.0 / PI;

    // Logika Tombol TARE RESET 2 DETIK
    static unsigned long buttonPressStartTime = 0;
    static bool resetTriggered = false;
    bool buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState == LOW) { 
      if (lastButton == HIGH) { 
        buttonPressStartTime = millis();
        resetTriggered = false;
      }
      
      if (!resetTriggered && (millis() - buttonPressStartTime >= 2000)) {
        pitch_bias = raw_pitch;
        roll_bias  = raw_roll;
        yaw_bias   = raw_yaw; // Menyimpan titik nol Yaw terbaru
        Serial.println(">>> POSISI DIRESET KE 0.0 (DITAHAN 2 DETIK) <<<");
        
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
        delay(300); 
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        
        resetTriggered = true; 
      }
    }
    lastButton = buttonState;

    // Output sudut setelah dikurangi bias tare
    float pitch = wrapAngle(raw_pitch - pitch_bias);
    float roll  = wrapAngle(raw_roll - roll_bias);
    float yaw   = wrapAngle(raw_yaw - yaw_bias);

    String nowTime = getTimeStamp();

    // Pengiriman Data ke Web Server via SSE Json
    if (nowMillis - lastSseSend > 10) {
      lastSseSend = nowMillis; 
      
      float totalAcc = sqrt(aX * aX + aY * aY + aZ * aZ);
      String status = (totalAcc > 20.0) ? "JATUH!" : "AMAN";

      StaticJsonDocument<512> doc;
      doc["time"] = nowMillis;
      doc["x"] = round(aX * 100) / 100.0;
      doc["y"] = round(aY * 100) / 100.0;
      doc["z"] = round(aZ * 100) / 100.0;
      doc["gx"] = round(gX * 100) / 100.0;
      doc["gy"] = round(gY * 100) / 100.0;
      doc["gz"] = round(gZ * 100) / 100.0;
      doc["total"] = round(totalAcc * 100) / 100.0;
      doc["roll"] = round(roll * 10) / 10.0;
      doc["pitch"] = round(pitch * 10) / 10.0;
      doc["yaw"] = round(yaw * 10) / 10.0; 
      doc["status"] = status;

      String payload;
      serializeJson(doc, payload);
      events.send(payload.c_str(), "sensor_data", millis());
    }
  } 
  
  delay(10);
}