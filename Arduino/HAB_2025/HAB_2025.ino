// Make sure the file system = Sketch: 1 MB FS: 7 MB FLASH
#include <LittleFS.h>  // The actual storage method. Pretty cool actually. 
#include <SPI.h>
#include <LoRa.h>  // Moved from RFM95. Easier to transmit structs, and JUST LoRa, not other modulations as well. 
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <Adafruit_LIS331HH.h>


// That's the average, if you want to be baller, change it to the actual pressure for the day. 
#define SEALEVELPRESSURE_HPA (1013.25) 
// Connections to the RFM95 on the board, check Adafruit documentation. 
#define RFM95_CS 16   
#define RFM95_INT 21
#define RFM95_RST 17
// It takes around 900 ms to complete a loop. So the sampling rate cannot be less than 900 ms. I guess it could, it would just run at 900 ms. 
#define SAMPLING_RATE 5000  
#define TRANSMIT_RATE 5000  
// Likely to change based off of antenna analyzation. 
#define FREQ 907.0

// All the files. 
File data_file; 
File log_file;

// All the timers. 
unsigned long current_millis = millis();
unsigned long last_data_save_millis;
unsigned long last_transmit_millis;

// All the sensors. 
RTC_DS3231 rtc;
Adafruit_LIS331HH lis = Adafruit_LIS331HH();
Adafruit_BME680 bme;

// All the... whitespace? 




// The actual struct. Notice how everything is int? We do some *magic* to convert everyhthing. 
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
  LittleFS.begin(); // MUST BE CALLED FIRST. Corruption may, no, WILL (probably most definitely), occur if called later. 
  delay(2000);      // Breathe....

  Serial.begin(115200); // Why do people use 9600? 

  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT); // Gotta use the correct pins, default pins are not for the board we are using. 

  // TODO: Add logging to make sure these initialize.
  if (!LoRa.begin(907E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // TODO: Add logging to make sure these initialize.
  rtc.begin();
  lis.begin_I2C();
  bme.begin();

  // Oversampling is important, more precise data at a faster rate, but higher power consumption. 
  // TBH, there's probaby other adverse side affects... 
  lis.setRange(LIS331HH_RANGE_12_G);
  lis.setDataRate(LIS331_DATARATE_50_HZ);
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_8X);
}

// Let's set up the struct so we can use it later. 
SensorData data;

void loop() {
  unsigned long current_millis = millis(); // Gotta keep the time! 

  // If it's been SAMPLING_RATE millis, take a sample. 
  if (current_millis - last_data_save_millis >= SAMPLING_RATE) {
    last_data_save_millis = current_millis;

    Serial.println("Saving data."); // Gotta love those serial sniffers... 

    DateTime now = rtc.now();
    sensors_event_t event;  // Adafruit_Sensor is supposed to be universal, but it only works for the lis. ¯\_(ツ)_/¯ (Makes my life harder...)
    lis.getEvent(&event);
    bme.performReading();

    // This is where the *magic* happens.
    // Floats are an unrealiable bunch, they "float" around, and if corrupted, are not easy to fix.
    // So, we convert everything to int. We multiply by the the amount of decimal places we need, and then round it off.
    // We can then divide them later bt the number of decimal places, and get the same precision up to the decimal places we wanted. 
    // we can also fit more into an int (RP2040's use int32_t by default, think 0xDEADBEEF, that's 32 bits.) than a float. 
    data.temp = round(bme.temperature * 100);
    data.humidity = round(bme.humidity * 100);
    data.pressure = round(bme.pressure * 100);
    data.altitude = round(bme.readAltitude(SEALEVELPRESSURE_HPA) * 100);
    data.accel_x = round(event.acceleration.x * 1000);
    data.accel_y = round(event.acceleration.y * 1000);
    data.accel_z = round(event.acceleration.z * 1000); 
    data.unixtime = now.unixtime();

    Serial.println(data.unixtime);

  // If it's been TRANSMIT_RATE millis, transmit! 
  // TODO: When it's not transmit time, send the radio to nap time. 
  // Unless we do a verification check... Then we gotta put it in read mode. 
  } else if (current_millis - last_transmit_millis >= TRANSMIT_RATE) {
    last_transmit_millis = current_millis;

    LoRa.beginPacket();
    LoRa.write((byte*)&data, sizeof(data));
    LoRa.endPacket();

    Serial.println("Transmiting.");
  }
}
