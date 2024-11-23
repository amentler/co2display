#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "sensor/SCD30.h"
#include <Preferences.h>
#include "buttons/Buttons.h"


void sleepDeeply(int);
void displayValues();
bool readSensor();
bool hasSensorValueChanged();
bool isInRange(int, int, int);
String getTrend(int, int);
void updateStatistics();
void checkBattery();

const int SERIAL_BITRATE = 115200;
const int DISPLAY_DIAGNOSTICS_OUTPUT_BITRATE = 0; //0 für keinen output

const int SLEEP_TIME = 300;

int lastHumidity = 0;
int lastTemperature = 0;
int lastCO2 = 0;

int currentHumidity = 0;
int currentTemperature = 0;
int currentCO2 = 0;

String humidityTrend = "-";
String cO2Trend = "-";
String temperatureTrend = "-";

const String UP = "/\\";
const String DOWN = "\\/";

Preferences preferences;

Buttons buttons;

GxEPD2_BW<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT> display(
  GxEPD2_154_GDEY0154D67(/*CS=D8*/ 0, /*DC=D3*/ 19, /*RST=D4*/ 21, /*BUSY=D2*/ 22)); // GDEY0154D67 200x200, SSD1681, (FPC-B001 20.05.21)

bool buttonWakeUp = false;
void setup() {  
  Serial.begin(115200);

  gpio_hold_dis(GPIO_NUM_2);

  buttons.init();

  Serial.println("Button Status:" + String(buttons.isButton1Pressed()));

  while(buttons.isButton1Pressed()){buttonWakeUp = true;};

  // Initialize the preferences library
  preferences.begin("co2sensor", false); // "my-app" is the namespace, false for read/write mode

  scd30.initialize();
  //Serial.println("SCD30 initialized");  

  display.init(DISPLAY_DIAGNOSTICS_OUTPUT_BITRATE, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse 
  //Serial.println("Display initialized");

  pinMode(A0, INPUT);
}

void loop() {
  runOnce();
}

void runOnce()
{
  blinkTwice();
  loadLastValues();
  checkBattery();

  if ((readSensor() && hasSensorValueChanged()) || buttonWakeUp)
  {
    updateStatistics();
    displayValues();
  }
  
  sleepDeeply(SLEEP_TIME);
}

void loadLastValues()
{
  lastCO2 = preferences.getInt("lastCO2", 0);
  lastHumidity = preferences.getInt("lastHumidity", 0);
  lastTemperature = preferences.getInt("lastTemperature", 0);

  Serial.println("Values loaded: lastCO2: " + String(lastCO2) + " / lastHumidity: " + String(lastHumidity) + " / lastTemperature: " + String(lastTemperature) );
}

void blinkTwice()
{
  Serial.println("blink");
  pinMode(LED_BUILTIN, OUTPUT);
  for (int blinkerCount = 0; blinkerCount<4; blinkerCount++) {
    analogWrite(LED_BUILTIN, INT_MAX);
    delay(1);
    analogWrite(LED_BUILTIN, 0);
    delay(10);
  }
}

const float MAX_READOUT = 4095;
const float MAX_VOLTAGE = 4.3;
String batteryVoltage = "0V";
void checkBattery() {
  Serial.println("Battery Analog Read: " + String(analogRead(A0)));
  batteryVoltage = String((float(analogRead(A0))/MAX_READOUT) * MAX_VOLTAGE) + "V";
  Serial.println("Battery Check: " + batteryVoltage);
}

void sleepDeeply(int sleepSeconds)
{
  int deepSleepMicroseconds = 1000 * 1000 * sleepSeconds;
  Serial.println("Beginning deep sleep for seconds: " + String(sleepSeconds));
  gpio_hold_en(GPIO_NUM_2);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 1);
  gpio_deep_sleep_hold_en();
  esp_deep_sleep(deepSleepMicroseconds);
}

bool hasSensorValueChanged() {
  return !(isInRange(currentCO2, lastCO2, 100) && isInRange(currentHumidity, lastHumidity, 2) && currentTemperature == lastTemperature);
}

void updateStatistics() {
  Serial.println("---updating Statistics!---");
  cO2Trend = getTrend(currentCO2, lastCO2);
  Serial.println("currentCO2:" + String(currentCO2) + " lastCO2:" + String(lastCO2) + " Trend: " + String(cO2Trend));
  lastCO2 = currentCO2;  

  humidityTrend = getTrend(currentHumidity, lastHumidity);
  Serial.println("currentHumidity:" + String(currentHumidity) + " lastHumidity:" + String(lastHumidity) + " Trend: " + String(humidityTrend));
  lastHumidity = currentHumidity;

  temperatureTrend = getTrend(currentTemperature, lastTemperature);
  Serial.println("currentTemperature:" + String(currentTemperature) + " lastTemperature:" + String(lastTemperature) + " Trend: " + String(temperatureTrend));
  lastTemperature = currentTemperature;

  Serial.println("Writing values to NVS");
  preferences.putInt("lastCO2", lastCO2);
  preferences.putInt("lastHumidity", lastHumidity);
  preferences.putInt("lastTemperature", lastTemperature);
}

String getTrend(int val1, int val2) {
  if (val1==val2) return "-";
  else if (val1>val2) return UP;
  else return DOWN;
}

bool isInRange(int value, int lastvalue, int deviation) {
  //Serial.println("isInRange value: " + String(value) + ", lastValue: " + String(lastvalue) + ", deviation: " + deviation); 
  return ((value - deviation) < lastvalue) && (lastvalue < (value + deviation)); 
}

bool readSensor() {
  Serial.println("Reading Sensor");
  if (scd30.isAvailable()) {
    float result[3] = {0};
    scd30.getCarbonDioxideConcentration(result);
    currentCO2 = (int)round(result[0]);
    currentTemperature = (int)round(result[1]);
    currentHumidity = (int)round(result[2]);
    Serial.println("currentCO2:" + String(currentCO2) + " currentTemperature:" + String(currentTemperature) + " currentHumidity: " + String(currentHumidity));
    return true;
  }
  return false;
}

void displayValues()
{
    Serial.println("updating Display!");

    String values = "\n" + String(currentCO2) + "ppm " + String(cO2Trend) + "\n \n" + 
                    String(currentTemperature) + "C " + String(temperatureTrend) + "\n \n" + 
                    String(currentHumidity) + "% " + String(humidityTrend) ;
    Serial.println(values);
        
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();
    display.firstPage();
    display.fillScreen(GxEPD_WHITE);
    display.setTextSize(3);

    display.setCursor(0, 0);
    display.print(values);

    display.setCursor(110, 173);
    display.print(batteryVoltage);
    display.nextPage();

    Serial.println("Display update is done");
}