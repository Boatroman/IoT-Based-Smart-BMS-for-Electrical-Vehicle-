#include <OneWire.h>
#include <DallasTemperature.h>

#define BLYNK_TEMPLATE_ID "TMPL3OcJLWhAT"
#define BLYNK_TEMPLATE_NAME "Smart IoT"
#define BLYNK_AUTH_TOKEN "xd0y7yla8e-64Xewa5433oGFUJtk2UnV"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#define USE_NODE_MCU_BOARD

#include "BlynkEdgent.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define ONE_WIRE_BUS_D5 14 // D5 for DS18B20
#define RELAY_PIN_D0 D0     // Define relay pin to D0
#define NUM_SENSORS 4       // Number of DS18B20 sensors

OneWire oneWire_D5(ONE_WIRE_BUS_D5);
DallasTemperature DS18B20_D5(&oneWire_D5);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
float voltage;
int bat_percentage;
int analogInPin = A0; // Analog input pin
int sensorValue;
float calibration = 0.0; // Check Battery voltage using multimeter & add/subtract the value
int relayState = LOW;    // Initial relay state

void setup() {
  Serial.begin(115200);
  delay(100);
  BlynkEdgent.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (128x64)
  display.clearDisplay();
  display.setTextColor(WHITE);
  delay(100);

  pinMode(RELAY_PIN_D0, OUTPUT);

  // Set the number of sensors found on the bus
  DS18B20_D5.begin();
  DS18B20_D5.setResolution(10); // Set the sensor resolution (you can adjust this if needed)

  // Search for DS18B20 devices and print their addresses
  Serial.println("Scanning for DS18B20 devices...");
  int deviceCount = DS18B20_D5.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 device(s)");
  if (deviceCount != NUM_SENSORS) {
    Serial.println("Error: Incorrect number of DS18B20 sensors detected!");
    while (1); // Stop execution if incorrect number of sensors found
  }
}

// Blynk function to control the relay
BLYNK_WRITE(V2) {
   int value = param.asInt();
   digitalWrite(RELAY_PIN_D0, value);
}

void loop() {
  BlynkEdgent.run();

  DS18B20_D5.requestTemperatures();

  // Print individual sensor data and send to Blynk
  for (int i = 0; i < NUM_SENSORS; ++i) {
    float temp = DS18B20_D5.getTempCByIndex(i);
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");
    Blynk.virtualWrite(V5 + i, temp); // Send individual sensor data to Blynk
  }

  // Calculate average temperature
  float totalTemp = 0;
  for (int i = 0; i < NUM_SENSORS; ++i) {
    totalTemp += DS18B20_D5.getTempCByIndex(i);
  }
  float averageTemp = totalTemp / NUM_SENSORS;
  sensorValue = analogRead(analogInPin);
  voltage = (((sensorValue * 27) / 1026) * 2 + calibration); // multiply by two as voltage divider
  bat_percentage = mapfloat(voltage, 37, 54.5, 0, 100);       // 37V as Battery Cut off Voltage & 54.5V as Maximum Voltage
  if (bat_percentage >= 100)
  {
    bat_percentage = 100;
  }
  if (bat_percentage <= 0)
  {
    bat_percentage = 1;
  }

  // Send data to Blynk
  Blynk.virtualWrite(V1, averageTemp); // for Temperature
  Blynk.virtualWrite(V3, voltage);            // for battery voltage
  Blynk.virtualWrite(V4, bat_percentage);     // for battery percentage

  // Print data on serial monitor
  Serial.print("averageTemperature: ");
  Serial.print(averageTemp);
  Serial.println(" °C");

  Serial.print("Analog Value = ");
  Serial.println(sensorValue);
  Serial.print("Output Voltage = ");
  Serial.println(voltage);
  Serial.print("Battery Percentage = ");
  Serial.println(bat_percentage);

  Serial.println();
  Serial.println("");
  Serial.println();
  delay(1000);

  if (bat_percentage <= 30)
  {
    Serial.println("Battery level below 30%, Charge battery on time");
    // send notification
    Blynk.logEvent("battery_low", "Battery is getting low.... Plugin to charge");
    delay(500);
  }

  // Display temperature on OLED
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(0.7);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 30);
  display.print("Temp: ");
  display.print(averageTemp);
  display.print(" *C");

  // Display voltage
  display.setCursor(0, 40);
  display.print("Voltage: ");
  display.print(voltage);
  display.print(" V");

  // Display soc
  display.setCursor(0, 50);
  display.print("S O C: ");
  display.print(bat_percentage);
  display.print(" %");

  display.display();
  delay(1500);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}