// Sketch: 1 MB FS: 7 MB FLASH
#include <LittleFS.h>
#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
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

#define RF95_FREQ 907.0


char csvBuffer[RH_RF95_MAX_MESSAGE_LEN];
uint8_t uint8Buffer[RH_RF95_MAX_MESSAGE_LEN];

int raw;

unsigned long current_millis = millis();
unsigned long last_data_save_millis;
unsigned long last_transmit_millis;


RTC_DS3231 rtc;
Adafruit_LIS331HH lis = Adafruit_LIS331HH();
Adafruit_BME680 bme;
RH_RF95 rf95(RFM95_CS, RFM95_INT);


void setup() {
  LittleFS.begin();
  delay(2000);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);

  data_file = LittleFS.open("/main/data/data_file.csv", "a");
  log_file = LittleFS.open("/main/logs/log.txt", "a");

  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  // Add logging to make sure these initialize.
  rf95.init();
  rtc.begin();
  lis.begin_I2C();
  bme.begin();

  lis.setRange(LIS331HH_RANGE_12_G);
  lis.setDataRate(LIS331_DATARATE_50_HZ);
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_8X);


  rf95.setFrequency(RF95_FREQ);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  rf95.setTxPower(23, false);

  log_file.close();
  data_file.close();
}


void loop() {
  unsigned long current_millis = millis();
  if (current_millis - last_data_save_millis >= SAMPLING_RATE) {
    last_data_save_millis = current_millis;
    data_file = LittleFS.open("/main/data/data_file.csv", "a");

    Serial.println("Saving data.");

    DateTime now = rtc.now();
    sensors_event_t event;
    lis.getEvent(&event);
    bme.performReading();

    raw = snprintf(csvBuffer, sizeof(csvBuffer), "%d, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n",
                   now.unixtime(), millis(), bme.temperature, bme.humidity, bme.pressure, 
                   bme.readAltitude(SEALEVELPRESSURE_HPA), event.acceleration.x, event.acceleration.y, event.acceleration.z);

    data_file.println(csvBuffer);
    Serial.println(csvBuffer);

    data_file.flush();
    data_file.close();

  } else if (current_millis - last_transmit_millis >= TRANSMIT_RATE) {
    last_transmit_millis = current_millis;

    Serial.println("Transmiting.");
    memcpy(uint8Buffer, csvBuffer, raw);
    delay(10);
    rf95.send(uint8Buffer, sizeof(uint8Buffer));
    delay(10);
  }
}
