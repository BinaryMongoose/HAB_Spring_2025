// Sketch: 1 MB FS: 7 MB FLASH
#include <LittleFS.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <Adafruit_LIS331HH.h>

File data_file;
File log_file;

#define SEALEVELPRESSURE_HPA (1013.25)

#define RFM95_CS 16
#define RFM95_INT 21
#define RFM95_RST 17
#define SAMPLING_RATE 5000  // It takes around 900 ms to complete a loop. So the sampling rate cannot be less than 900 ms.
#define TRANSMIT_RATE 5000  //60000

#define FREQ 907.0

unsigned long current_millis = millis();
unsigned long last_data_save_millis;
unsigned long last_transmit_millis;


RTC_DS3231 rtc;
Adafruit_LIS331HH lis = Adafruit_LIS331HH();
Adafruit_BME680 bme;

struct SensorData {
  int unixtime;
  int millis; 
  int temp;       
  int humidity;   
  int pressure;   
  int altitude;   
  int accel_x;
  int accel_y;
  int accel_z; 
};


void setup() {
  LittleFS.begin();
  delay(2000);

  Serial.begin(115200);

  
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT); 
  // Add logging to make sure these initialize.
  if (!LoRa.begin(907E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }


  rtc.begin();
  lis.begin_I2C();
  bme.begin();

  lis.setRange(LIS331HH_RANGE_12_G);
  lis.setDataRate(LIS331_DATARATE_50_HZ);
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_8X);
}


SensorData data;
void loop() {
  unsigned long current_millis = millis();
  if (current_millis - last_data_save_millis >= SAMPLING_RATE) {
    last_data_save_millis = current_millis;

    Serial.println("Saving data.");

    DateTime now = rtc.now();
    sensors_event_t event;
    lis.getEvent(&event);
    bme.performReading();

    data.temp = round(bme.temperature * 100);
    data.humidity = round(bme.humidity * 100);
    data.pressure = round(bme.pressure * 100);
    data.altitude = round(bme.readAltitude(SEALEVELPRESSURE_HPA) * 100);
    data.accel_x = round(event.acceleration.x * 1000);
    data.accel_y = round(event.acceleration.y * 1000);
    data.accel_z = round(event.acceleration.z * 1000); 
    data.unixtime = now.unixtime();

    Serial.println(event.acceleration.x); 

    Serial.println(data.unixtime);

  } else if (current_millis - last_transmit_millis >= TRANSMIT_RATE) {
    last_transmit_millis = current_millis;

    LoRa.beginPacket();
    LoRa.write((byte*)&data, sizeof(data));
    LoRa.endPacket();

    Serial.println("Transmiting.");
  }
}
