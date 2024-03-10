#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
// #include <esp_bt.h>

// Time
#include <NTPClient.h>

// Network
#include <WiFiUdp.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Arduino and GY-302 (light) module
// https://www.phippselectronics.com/using-gy-302-digital-light-intensity-sensor-module-with-arduino/
// I2C SCL 22
// I2C SDA 21
#include <Wire.h>
#include <BH1750.h>

#define SOIL_PIN 34  // ADC1
#define RAIN_PIN 27 // ADC2
#define LIGHT_SCL 22
#define LIGHT_SDA 21
#define SLEEP_TIME 60  // in seconds
#define WIFI_RETRY_TIMES 60

const char *SSID     = "REDACTED";
const char *WIFI_PASSWORD = "REDACTED";
const String SERVER_ADDR = "REDACTED";

String epochTime = "0";
String soil_humidity = "50";
String rain = "0";
String light_level = "0";
String other_info = "from_ESP32_v2";

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionally you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 0, 60000); // for UTC+8, 8 * 3600 = 28800

BH1750 GY302; // light

// Function declarations
String getSoil();
String getRain();
String getLight();
String getInternetTime();
void uploadData(String epochTime, String soil_humidity, String rain, String light_level, String other_info);
void gotoSleep(int seconds);

void setup() {
  Serial.begin(115200);
  delay(1000);  // Delay for serial port to initialize

  WiFi.disconnect(true); // WiFi signal off
  // Configure ADC
  analogReadResolution(12);  // Set ADC resolution to 12 bits
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(RAIN_PIN, ADC_11db);

  // Get data
  soil_humidity = getSoil();
  delay(1000);
  rain = getRain();
  delay(1000);
  light_level = getLight();
  delay(1000);

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  esp_wifi_start();
  delay(1000);

  int counter = 0;
  WiFi.begin(SSID, WIFI_PASSWORD);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED && counter < WIFI_RETRY_TIMES)
  {
    delay(1000);
    Serial.print(".");
    counter++;
  }
  if (counter == WIFI_RETRY_TIMES)
  {
    Serial.println("Connection to WiFi timeout");
    Serial.println("Please check your WiFi settings");
    Serial.println("Exiting...");
  }

  // get Time
  epochTime = getInternetTime();

  // Upload
  uploadData(epochTime, soil_humidity, rain, light_level, other_info);

  // Sleep
  gotoSleep(SLEEP_TIME);
}

void loop() {
  // Do nothing
}

String getSoil() {
  int sensorValue = analogRead(SOIL_PIN);

  if (isnan(sensorValue) || sensorValue == 0) {
    Serial.println(F("Error reading soil humidity!"));
    return "ERR";
  }
  else {
    Serial.printf("Soil humidity at: %d\n", sensorValue);
    return String(sensorValue);
  }
}

String getRain() {
  int sensorValue = analogRead(RAIN_PIN);

  if (isnan(sensorValue) || sensorValue == 0) {
    Serial.println(F("Error reading rain!"));
    return "ERR";
  }
  else {
    Serial.printf("Rain at: %d\n", sensorValue);
    return String(sensorValue);
  }
}

String getLight() {
  // I2C
  Wire.begin(LIGHT_SDA, LIGHT_SCL);
  delay(1000);

  GY302.begin();
  if (GY302.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  } else {
    Serial.println(F("Error initialising BH1750"));
    return "ERR_init";
  }
  delay(1000);

  if (GY302.measurementReady()) {
    float lux = GY302.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    return String(lux);
  }
  else {
    Serial.println(F("Error reading light!"));
    return "ERR";
  }
}

String getInternetTime() {
  // Get time
  // https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/

  timeClient.begin();
  delay(500);
  timeClient.update();

  String s_time = String(timeClient.getEpochTime());
  Serial.print(F("\nCurrent epoch time is:"));
  Serial.println(s_time);
  return s_time;
}

void uploadData(String epochTime, String soil_humidity, String rain, String light_level, String other_info) {
  Serial.println("--- The following should be uploaded ---");
  Serial.print("/update_weather?time=");
  Serial.print(epochTime);
  Serial.print("&soil_humidity=");
  Serial.print(soil_humidity);
  Serial.print("&rain=");
  Serial.print(rain);
  Serial.print("&light_level=");
  Serial.print(light_level);
  Serial.print("&other_info=");
  Serial.println(other_info);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String baseUrl = SERVER_ADDR + "/update_weather";
    String url = baseUrl + "?time=" + epochTime + "&soil_humidity=" + soil_humidity + "&rain=" + rain + \
                                                  "&light_level=" + light_level + "&other_info=" + other_info;

    Serial.println(F("--- Sending HTTP GET request... ---"));
    http.begin(client, url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("HTTP GET Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  delay(1000);

}

void gotoSleep(int seconds) {
  // Configure deep sleep
  esp_sleep_enable_timer_wakeup(seconds * 1000000);  // Set deep sleep wake time to 15 seconds
  // esp_bt_controller_disable();  // Disable Bluetooth
  esp_wifi_stop();  // Stop WiFi

  Serial.println("Setup complete. Entering deep sleep...");
  delay(500);  // Delay to allow printing to complete before entering deep sleep

  // Enter deep sleep
  esp_deep_sleep_start();
}