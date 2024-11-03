#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "sensor/SCD30.h"
#include <Preferences.h>


void sleepDeeply(int);
void displayValues();
bool readSensor();
bool hasSensorValueChanged();
bool isInRange(int, int, int);
String getTrend(int, int);
void updateStatistics();

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

GxEPD2_BW<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT> display(
  GxEPD2_154_GDEY0154D67(/*CS=D8*/ 0, /*DC=D3*/ 19, /*RST=D4*/ 21, /*BUSY=D2*/ 22)); // GDEY0154D67 200x200, SSD1681, (FPC-B001 20.05.21)


void setup() {  
  Serial.begin(115200);

  // Initialize the preferences library
  preferences.begin("co2sensor", false); // "my-app" is the namespace, false for read/write mode

  scd30.initialize();
  //Serial.println("SCD30 initialized");  

  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse 
  //Serial.println("Display initialized");

  blinkTwice();
}

void loadLastValues()
{
  lastCO2 = preferences.getInt("lastCO2", 0);
  lastHumidity = preferences.getInt("lastHumidity", 0);
  lastTemperature = preferences.getInt("lastTemperature", 0);

  Serial.println("Values loaded: lastCO2: " + String(lastCO2) + " / lastHumidity: " + String(lastHumidity) + " / lastTemperature: " + String(lastTemperature) );
}

void loop() {
  runOnce();
}

void runOnce()
{
  blinkTwice();
  loadLastValues();
  if (readSensor() && hasSensorValueChanged())
  {
    updateStatistics();
    displayValues();
  }

  for (int sleeperCount = 0; sleeperCount<4; sleeperCount++) {
    blinkTwice();
    sleepDeeply(60);
  }
}

void blinkTwice()
{
  Serial.println("blink");
  gpio_hold_dis(GPIO_NUM_2);
  pinMode(LED_BUILTIN, OUTPUT);
  for (int blinkerCount = 0; blinkerCount<4; blinkerCount++) {
    analogWrite(LED_BUILTIN, INT_MAX);
    delay(1);
    analogWrite(LED_BUILTIN, 0);
    delay(10);
  }
}

void sleepDeeply(int sleepSeconds)
{
  gpio_hold_en(GPIO_NUM_2);
  gpio_deep_sleep_hold_en();
  esp_deep_sleep(1000 * 1000 * sleepSeconds);
}

bool hasSensorValueChanged() {
  return !(isInRange(currentCO2, lastCO2, 100) && isInRange(currentHumidity, lastHumidity, 2) && currentTemperature == lastTemperature);
}

void updateStatistics() {
  Serial.println("updating Statistics!");
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
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(0, 0);
        display.setTextSize(3);
        display.print(values);
    } while (display.nextPage());
    Serial.println("Display update is done");
}