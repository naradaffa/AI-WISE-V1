#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <WiFi.h>
//#include <SPI.h>  
//#include <SD.h>   
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> 
#include "time.h"        

const char* ssid     = "NAMA_WIFI_KAMU_DISINI";
const char* password = "PASSWORD_WIFI_KAMU_DISINI";

AsyncWebServer server(80);
AsyncEventSource events("/events");

//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = 7 * 3600;
//const int   daylightOffset_sec = 0;

#define SDA_PIN    D4
#define SCL_PIN    D5
#define LED_RED    D6  
#define LED_GREEN  D7  
#define BUTTON_PIN D3

//#define SD_CS      D0 
//File dataFile;      

//String dataBuffer = "";     
//int bufferCounter = 0;      
//const int MAX_BUFFER = 30;  

MPU9250_asukiaaa mpu;

float aX_bias = 0, aY_bias = 0, aZ_bias = 0;
float mX_bias = 0, mY_bias = 0, mZ_bias = 0;
float roll_bias = 0, pitch_bias = 0, yaw_bias = 0;

unsigned long lastTime = 0;
unsigned long lastSseSend = 0; 
bool lastButton = HIGH;

float wrapAngle(float a) {
  while (a >  180) a -= 360;
  while (a < -180) a += 360;
  return a;
}

String getTimeStamp() {
  //struct tm timeinfo;
  //if (!getLocalTime(&timeinfo)) {
  //  return "TIME_ERR"; 
  //}
  //char buffer;
  //strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &timeinfo);
  //return String(buffer);
  return String(millis());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);   
  pinMode(LED_GREEN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  /* if (!SD.begin(SD_CS)) {
    Serial.println("SD Card gagal!");
  } else {
    Serial.println("SD Card siap!");
    if (!SD.exists("/data.csv")) {
      dataFile = SD.open("/data.csv", FILE_WRITE);
      if (dataFile) {
        dataFile.println("time,ax,ay,az,gx(deg/s),gy(deg/s),gz(deg/s),roll,pitch,yaw");
        dataFile.close();
      }
    }
  }
  */

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

  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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

  Serial.println("\n[2/2] >>> PUTAR ALAT SEKARANG! 360 DERAJAT DATAR (Waktu 5 Detik) <<<");
  float my_min = 32000, my_max = -32000;
  float mz_min = 32000, mz_max = -32000;
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
      float my = mpu.magY(); 
      float mz = mpu.magZ();
      if (my < my_min) my_min = my; if (my > my_max) my_max = my;
      if (mz < mz_min) mz_min = mz; if (mz > mz_max) mz_max = mz;
    }
    delay(10); 
  }
  
  mY_bias = (my_max + my_min) / 2.0;
  mZ_bias = (mz_max + mz_min) / 2.0; 

  Serial.println("\n>>> BERHENTI MEMUTAR! HADAPKAN KE DEPAN & DIAMKAN <<<");
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(2000);

  mpu.magUpdate();
  yaw_bias = atan2(mpu.magZ() - mZ_bias, mpu.magY() - mY_bias) * 180.0 / PI; 

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

  static float raw_yaw = 0; 
  if (statusMag == 0) {
    float mY_bersih = mpu.magY() - mY_bias;
    float mZ_bersih = mpu.magZ() - mZ_bias;
    raw_yaw = atan2(mZ_bersih, mY_bersih) * 180.0 / PI;
  }

  if (statusAccel == 0 && statusGyro == 0) {

    float aX = (mpu.accelX() * 9.80665) - aX_bias;
    float aY = (-mpu.accelZ() * 9.80665) - aY_bias; 
    float aZ = (-mpu.accelY() * 9.80665) - aZ_bias;

    float gX = mpu.gyroX(); 
    float gY = -mpu.gyroZ(); 
    float gZ = mpu.gyroY();

    float raw_pitch = atan2(aX, sqrt(aY * aY + aZ * aZ)) * 180.0 / PI; 
    float raw_roll  = -atan2(aY, sqrt(aX * aX + aZ * aZ)) * 180.0 / PI;

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
        yaw_bias   = raw_yaw;
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

    float pitch = wrapAngle(raw_pitch - pitch_bias);
    float roll  = wrapAngle(raw_roll - roll_bias);
    float yaw   = wrapAngle(raw_yaw - yaw_bias);

    String nowTime = getTimeStamp();

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

    /*
    String dataRow = nowTime + "," + String(aX, 2) + "," + String(aY, 2) + "," + String(aZ, 2) + "," + 
                     String(gX, 2) + "," + String(gY, 2) + "," + String(gZ, 2) + "," + 
                     String(roll, 2) + "," + String(pitch, 2) + "," + String(yaw, 2) + "\n";
    
    dataBuffer += dataRow;
    bufferCounter++;

    if (bufferCounter >= MAX_BUFFER) {
      dataFile = SD.open("/data.csv", FILE_APPEND);
      if (dataFile) {
        dataFile.print(dataBuffer);
        dataFile.close();
        dataBuffer = ""; 
        bufferCounter = 0;
      } else {
        Serial.println("Gagal nulis ke SD!");
      }
    }
    */
  } 
  
  delay(10);
}
