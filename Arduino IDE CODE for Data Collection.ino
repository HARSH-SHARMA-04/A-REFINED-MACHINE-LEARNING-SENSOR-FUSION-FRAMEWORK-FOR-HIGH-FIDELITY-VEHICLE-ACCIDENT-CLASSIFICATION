#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_Sensor.h>
#include <TinyGPSPlus.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID     = "vivo 1904";
const char* WIFI_PASSWORD = "qwerty5689";
const char* SHEETS_URL    = "https://script.google.com/macros/s/AKfycbzVaolVezNA4utP2xVT1mTqhbzOUNHTo9zhhB07WgviFILu17WNWOkVPlTYwDwYmYnNWA/exec";

Adafruit_MPU6050         mpu;
Adafruit_BMP085          bmp;
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

TinyGPSPlus gps;

#define SENSOR_INTERVAL   20
#define SHEET_INTERVAL    1000
#define ALPHA             0.98
#define SEA_LEVEL_HPA     1013.25
#define ALA_THRESHOLD     25.0
#define SPEED_THRESHOLD   2.0
#define ALT_DROP_THRESH   1.0
#define GYRO_THRESHOLD    2.0
#define ROLLOVER_THRESH   90.0

float pitch        = 0.0;
float roll         = 0.0;

float prevRowAltitude = 0.0;
float altitudeChange  = 0.0;
float altitudeDrop    = 0.0;
bool  firstRow        = true;

float         maxALA      = 0.0;
float         recordedALA = 0.0;
unsigned long windowStart = 0;

int highALAcount = 0;

unsigned long lastSensorRead  = 0;
unsigned long lastSheetUpdate = 0;
unsigned long lastGyroTime    = 0;

float lastLat   = 0.0;
float lastLng   = 0.0;
float lastSpeed = 0.0;
int   lastSats  = 0;
bool  gpsFixed  = false;

float baselineAlt = 0.0;
bool  baselineSet = false;

void feedGPS() {
  while (Serial.available()) {
    gps.encode(Serial.read());
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  delay(500);

  Wire.begin(4, 5);

  Serial1.println("\n=== ACCIDENT DETECTION V1 (Google Sheets) ===");

  if (!mpu.begin()) {
    Serial1.println("MPU6050 FAILED — check wiring");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
  Serial1.println("MPU6050 OK");

  if (!bmp.begin()) {
    Serial1.println("BMP180 FAILED — check wiring");
    while (1);
  }
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += bmp.readAltitude(SEA_LEVEL_HPA);
    delay(50);
  }
  float baselineAltitude = sum / 10.0;
  prevRowAltitude = 0.0;
  Serial1.print("BMP180 OK — baseline: ");
  Serial1.println(baselineAltitude);

  if (!mag.begin()) {
    Serial1.println("HMC5883L WARNING — continuing without mag");
  } else {
    Serial1.println("HMC5883L OK");
  }

  Serial1.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    feedGPS();
    delay(500);
    Serial1.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial1.println("\nWiFi OK: " + WiFi.localIP().toString());
  } else {
    Serial1.println("\nWiFi FAILED — running offline");
  }

  lastGyroTime   = millis();
  windowStart    = millis();
  lastSensorRead = millis();

  Serial1.println(">> System ready at 50Hz\n");
}

void loop() {
  feedGPS();

  if (gps.location.isValid() && gps.location.age() < 2000) {
    lastLat  = gps.location.lat();
    lastLng  = gps.location.lng();
    gpsFixed = true;
  }
  if (gps.speed.isValid() && gps.speed.age() < 2000) {
    lastSpeed = gps.speed.kmph();
  }
  if (gps.satellites.isValid()) {
    lastSats = gps.satellites.value();
  }

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float alaRaw = sqrt(
      a.acceleration.x * a.acceleration.x +
      a.acceleration.y * a.acceleration.y +
      a.acceleration.z * a.acceleration.z
    );

    if (millis() - windowStart < 10) {
      if (alaRaw > maxALA) maxALA = alaRaw;
    } else {
      recordedALA = maxALA;
      maxALA      = 0.0;
      windowStart = millis();
    }

    if (recordedALA > ALA_THRESHOLD) {
      highALAcount++;
    } else {
      highALAcount = 0;
    }

    float accPitch = atan2(
      -a.acceleration.x,
      sqrt(a.acceleration.y * a.acceleration.y +
           a.acceleration.z * a.acceleration.z)
    ) * 180.0 / PI;

    float accRoll = atan2(
      a.acceleration.y,
      sqrt(a.acceleration.x * a.acceleration.x +
           a.acceleration.z * a.acceleration.z)
    ) * 180.0 / PI;

    unsigned long now = millis();
    float dt = (now - lastGyroTime) / 1000.0;
    if (dt > 0.1) dt = 0.02;
    lastGyroTime = now;

    pitch = ALPHA * (pitch + g.gyro.y * 180.0 / PI * dt)
            + (1.0 - ALPHA) * accPitch;
    roll  = ALPHA * (roll  + g.gyro.x * 180.0 / PI * dt)
            + (1.0 - ALPHA) * accRoll;

    float absAltitude = bmp.readAltitude(SEA_LEVEL_HPA);
    float pressure    = bmp.readPressure() / 100.0;
    float bmpTemp     = bmp.readTemperature();

    if (!baselineSet) {
      baselineAlt = absAltitude;
      baselineSet = true;
    }

    float currentAltitude = absAltitude - baselineAlt;

    if (firstRow) {
      altitudeChange  = 0.0;
      altitudeDrop    = 0.0;
      prevRowAltitude = currentAltitude;
      firstRow        = false;
    } else {
      altitudeChange  = abs(currentAltitude - prevRowAltitude);
      altitudeDrop    = prevRowAltitude - currentAltitude;
      prevRowAltitude = currentAltitude;
    }

    float gyroMag = sqrt(
      g.gyro.x * g.gyro.x +
      g.gyro.y * g.gyro.y +
      g.gyro.z * g.gyro.z
    );

    String timestamp = "";
    if (gps.time.isValid()) {
      char buf[20];
      sprintf(buf, "%02d:%02d:%02d",
              gps.time.hour(),
              gps.time.minute(),
              gps.time.second());
      timestamp = String(buf);
    } else {
      timestamp = String(millis() / 1000) + "s";
    }

    String accidentType = "NO ACCIDENT";

    if (abs(pitch) >= ROLLOVER_THRESH || abs(roll) >= ROLLOVER_THRESH) {
      accidentType = "ROLLOVER";
    } else if (recordedALA > ALA_THRESHOLD && lastSpeed < SPEED_THRESHOLD) {
      if (altitudeDrop > ALT_DROP_THRESH || gyroMag > GYRO_THRESHOLD) {
        accidentType = "FALLOFF";
      } else {
        accidentType = "COLLISION";
      }
    }

    Serial1.println("---------- READINGS ----------");
    Serial1.print("ALA      : "); Serial1.print(recordedALA, 3); Serial1.println(" m/s²");
    Serial1.print("ALA_g    : "); Serial1.print(recordedALA/9.80665, 3); Serial1.println(" g");
    Serial1.print("Pitch    : "); Serial1.print(pitch, 2); Serial1.println(" deg");
    Serial1.print("Roll     : "); Serial1.print(roll, 2);  Serial1.println(" deg");
    Serial1.print("AltRel   : "); Serial1.print(currentAltitude, 3); Serial1.println(" m");
    Serial1.print("AltChg   : "); Serial1.print(altitudeChange, 3); Serial1.println(" m");
    Serial1.print("AltDrop  : "); Serial1.print(altitudeDrop, 3);  Serial1.println(" m");
    Serial1.print("GyroMag  : "); Serial1.print(gyroMag, 3); Serial1.println(" rad/s");
    Serial1.print("Speed    : "); Serial1.print(lastSpeed, 2); Serial1.println(" km/h");
    Serial1.print("GPS Fix  : "); Serial1.println(gpsFixed ? "YES" : "NO");
    Serial1.print("Sats     : "); Serial1.println(lastSats);
    Serial1.print("Accident : "); Serial1.println(accidentType);
    Serial1.println();

    if (millis() - lastSheetUpdate >= SHEET_INTERVAL) {
      lastSheetUpdate = millis();

      if (WiFi.status() == WL_CONNECTED) {
        feedGPS();

        String json = "{";
        json += "\"timestamp\":\"" + timestamp + "\",";
        json += "\"accX\":"     + String(a.acceleration.x, 3) + ",";
        json += "\"accY\":"     + String(a.acceleration.y, 3) + ",";
        json += "\"accZ\":"     + String(a.acceleration.z, 3) + ",";
        json += "\"ala\":"      + String(recordedALA, 3)      + ",";
        json += "\"gyroX\":"    + String(g.gyro.x, 4)         + ",";
        json += "\"gyroY\":"    + String(g.gyro.y, 4)         + ",";
        json += "\"gyroZ\":"    + String(g.gyro.z, 4)         + ",";
        json += "\"pitch\":"    + String(pitch, 2)            + ",";
        json += "\"roll\":"     + String(roll, 2)             + ",";
        json += "\"altitude\":" + String(currentAltitude, 3)  + ",";
        json += "\"altChange\":" + String(altitudeChange, 3)  + ",";
        json += "\"altDrop\":"  + String(altitudeDrop, 3)     + ",";
        json += "\"gyroMag\":"  + String(gyroMag, 3)          + ",";
        json += "\"pressure\":" + String(pressure, 2)         + ",";
        json += "\"temp\":"     + String(bmpTemp, 1)          + ",";
        json += "\"gpsSpeed\":" + String(lastSpeed, 2)        + ",";
        json += "\"latitude\":" + String(lastLat, 6)          + ",";
        json += "\"longitude\":" + String(lastLng, 6)         + ",";
        json += "\"satellites\":" + String(lastSats)          + ",";
        json += "\"accidentType\":\"" + accidentType          + "\"";
        json += "}";

        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setTimeout(8000);
        http.begin(client, SHEETS_URL);
        http.addHeader("Content-Type", "application/json");

        feedGPS();
        int code = http.POST(json);
        Serial1.print("HTTP: "); Serial1.println(code);
        http.end();

        feedGPS();
      }
    }
  }
}
