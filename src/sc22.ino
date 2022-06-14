// Setup:
// - ESP32-S2 board setup:- https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/arduino-ide-setup
// - TFT setup: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/built-in-tft
// - Adafruit IO setup: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/usage-with-adafruit-io
//
// Sensor setup: install the following libraries with dependencies:
// - arduino-sht
// - Sensirion I2C SCD4x
//
// this is a simple demonstration to show a prototyping setup; this
// is not meant to be used in a production setup without additional
// error checking/handling
//
// LED Codes
// 1. LED off: during initialization
// 2. LED on permanently: init done
// 3. LED blinking: panic; reset board
//
// TODO:
// - create a dynamic baseline calculation, to avoid staying in red during the show
// - camel case vs underscores
// - do we need IO reconnect?
// 
// Questions: email Johannes Winkelmann, jwi@sensirion.com


#include "config.h"
// ^^ create a local file called wifi_config.h, and put the following info
// ------
// #define WIFI_SSID   "your wifi ssid"            // your network SSID (name)
// #define WIFI_PASS   "your wifi pass"            // your network password (use for WPA, or use as key for WEP)
//
// #define IO_USERNAME "your adafruit io username"
// #define IO_KEY      "your adafruit io key"
// ------


// -- demo configuration
#define USE_NETWORK 1             // enable network upload
#define USE_F       1             // output temperature in Fahrenheit (alternative: Celcius)

#define CO2_THRESHOLD_RED    1200 // CO2 threshold for "orange" warning
#define CO2_THRESHOLD_ORANGE 1000 // CO2 threshold for "red" alarm

// Adafruit IO configuration
#define IO_UPLOAD_INTERVAL_S 10   // interval for cloud upload in seconds
#define IO_CO2_CHANNEL "sc22_co2_1" // Channel name for CO2
#define IO_RH_CHANNEL   "sc22_rh_1" // Channel name for relative humidity
#define IO_T_CHANNEL     "sc22_t_1" // Channel name for temperature

// display configuration
#define USE_DISPLAY 1             // enable display on ESP32-S2 TFT

#define FONT_SIZE     3
#define TITLE_OFFSET 10
#define LINE_OFFSET  30

// -- end demo configuration don't edit past this point unless you want to modify the demo



#if USE_DISPLAY
  #include <Adafruit_GFX.h>    // Core graphics library
  #include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
  #include <SPI.h>
#endif


#include <SHTSensor.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

#if USE_NETWORK
  #include <WiFi.h>
  #include <AdafruitIO_WiFi.h>

  AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

  // set up the Adafruit IO feeds
  AdafruitIO_Feed* io_co2         = io.feed(IO_CO2_CHANNEL);
  AdafruitIO_Feed* io_humidity    = io.feed(IO_RH_CHANNEL);
  AdafruitIO_Feed* io_temperature = io.feed(IO_T_CHANNEL);
#endif

bool io_connected = false;

#if USE_DISPLAY
  // Use dedicated hardware SPI pins
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#endif

// Sensors
SHTSensor sht;
SensirionI2CScd4x scd4x;

// Sensor data and error handling
uint16_t scdError;
char     scdErrorMessage[256];
uint16_t scd_co2 = 0;
float    scd_t, scd_rh;


#ifdef USE_F
  #define LOCALTEMP(T) (T * 1.8 + 32)
#else
  #define LOCALTEMP(T) (T)
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  initTFT();
  delay(500);

  if (!initSensors()) {
    panic();
    exit(1);
  }

  displayTitle();
  initAdafruitIO();
  displayTitle();

  digitalWrite(LED_BUILTIN, HIGH);
}

void initAdafruitIO()
{
#if USE_NETWORK
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  io_connected = true;
#endif
}

void initTFT()
{
#if USE_DISPLAY
    // turn on backlight
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  Serial.println(F("Initialized"));

  tft.fillScreen(ST77XX_BLACK);
#endif
}

bool initSensors()
{
  Wire.begin();
  if (sht.init()) {
    Serial.print("init(): success\n");
  } else {
    Serial.print("init(): failed\n");
    return false;
  }

  scd4x.begin(Wire);
  // stop potentially previously started measurement
  scdError = scd4x.stopPeriodicMeasurement();
  if (scdError) {
      Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(scdError, scdErrorMessage, 256);
      Serial.println(scdErrorMessage);
  }

  scdError = scd4x.startPeriodicMeasurement();
  if (scdError) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(scdError, scdErrorMessage, 256);
      Serial.println(scdErrorMessage);
      return false;
  }

  return true;
}

// ---- post init

void loop() {
  static uint8_t loop_counter = 0;
  ++loop_counter; // used for rate limiting
  
  // clear old sensor data
  displaySensorData(true, scd_co2, LOCALTEMP(sht.getTemperature()), sht.getHumidity());

  if (!sht.readSample()) {
      Serial.print("Error in readSample()\n");
  }

  if (loop_counter % 5 == 0) { // execute every 5s
    scdError = scd4x.readMeasurement(scd_co2, scd_t, scd_rh);
    if (scdError) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(scdError, scdErrorMessage, 256);
        Serial.println(scdErrorMessage);
    } else if (scd_co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        
    }
  }

  displaySensorData(false, scd_co2, LOCALTEMP(sht.getTemperature()), sht.getHumidity());
  if (loop_counter % IO_UPLOAD_INTERVAL_S == 0) { // execute every UPLOAD_INTERVAL_S
    uploadToIO(scd_co2, LOCALTEMP(sht.getTemperature()), sht.getHumidity());
  }

  // create a plotable output; scaling T and RH to be in a similar range to CO2
  Serial.print("CO2:");
  Serial.print(scd_co2);

  Serial.print(",T:");
  Serial.print((int)(LOCALTEMP(sht.getTemperature()) * 10));

  Serial.print(",RH:");
  Serial.println((int)(sht.getHumidity() * 10));
  

  delay(1000);
}

// -- Adafruit IO
bool uploadToIO(float co2, float t, float rh)
{
  bool success = true;

#if USE_NETWORK
  success = io_co2->save(co2);
  success &= io_temperature->save(t);
  success &= io_humidity->save(rh);

  if (!success) {
    Serial.print("Error uploading to IO\n");

    Serial.print("IO status\n");
    Serial.println(io.status());
    
    Serial.print("IO network status\n");
    Serial.println(io.networkStatus());

    io_connected = false;
  }
#endif

  return success;
}

// -- Display stuff
void displayTitle()
{
#if USE_DISPLAY
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(io_connected ? ST77XX_CYAN : ST77XX_YELLOW);
  tft.setTextWrap(true);
  tft.setTextSize(FONT_SIZE);
  tft.print("Converge Demo");
#endif
}

void displaySensorData(bool clear, float co2, float t, float rh)
{
#if USE_DISPLAY
  tft.setTextColor(clear ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setCursor(0, TITLE_OFFSET + LINE_OFFSET);
  tft.print("T:    ");
  tft.print(t);
#if USE_F
  tft.print(" F");
#else
  tft.print(" C");
#endif

  tft.setTextColor(clear ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setCursor(0, TITLE_OFFSET + 2 * LINE_OFFSET);
  tft.print("RH:   ");
  tft.print(rh);
  tft.print(" %");

  uint16_t co2_color = ST77XX_GREEN;
  if (co2 > CO2_THRESHOLD_RED) {
    co2_color = ST77XX_RED;
  } else if (co2 > CO2_THRESHOLD_ORANGE) {
    co2_color = ST77XX_ORANGE;
  }
  tft.setTextColor(clear ? ST77XX_BLACK : co2_color);
  tft.setCursor(0, TITLE_OFFSET + 3 * LINE_OFFSET);
  tft.print("CO2:");
  tft.print((int)co2);
  tft.print(" ppm");
#endif
}

void panic()
{
  Serial.println("PANIC: looping indefinitely");

  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

