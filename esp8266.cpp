#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <DHT_U.h>

// Wifi/Time
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define LED_PIN 2
#define DHT_PIN 12
#define DHTTYPE   DHT11
#define SLEEP_TIME 60 // in seconds
#define WIFI_RETRY_TIMES 60

const char *SSID     = "REDACTED";
const char *WIFI_PASSWORD = "REDACTED";
const String SERVER_ADDR = "REDACTED";

DHT_Unified dht(DHT_PIN, DHTTYPE);
struct DHT11Data {
  String temp;
  String air_humidity;
};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 0, 60000); // 如果要直接做UTC+8 = 8 * 3600 = 28800，改第三个参数
Adafruit_BMP085 bmp;

// Function declarations
DHT11Data getDHT11Data();
String getAirPressure();
String getInternetTime();
void uploadData(String epochTime, DHT11Data DHT_data, String air_pressure, String other_info);

void setup() {
  Serial.begin(9600);

  // Wait for serial to initialize.
  // while(!Serial) { }
  delay(1000);

  // Connect to wifi
  int counter = 0; // Wifi retry counter
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
  
  Serial.println(F("-----------------"));
  Serial.println(F("Booting completed."));
}

void loop() {
  String epochTime = getInternetTime();
  DHT11Data DHT_data = getDHT11Data();
  String air_pressure = getAirPressure();
  String other_info = "from_ESP8266_v2";
  uploadData(epochTime, DHT_data, air_pressure, other_info);

  Serial.println(F("Delay some seconds......"));
  delay(SLEEP_TIME * 1000);
}

DHT11Data getDHT11Data() {
  DHT11Data sensorData;

  dht.begin();
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    sensorData.temp = "ERR";
  }
  else {
    sensorData.temp = String(event.temperature);
    Serial.print(F("Temperature: "));
    Serial.print(sensorData.temp);
    Serial.println(F("°C"));
  }

  delay(1000);

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading air humidity!"));
    sensorData.air_humidity = "ERR";
  }
  else {
    sensorData.air_humidity = String(event.relative_humidity);
    Serial.print(F("Humidity: "));
    Serial.print(sensorData.air_humidity);
    Serial.println(F("%"));
  }

  return sensorData;
}


String getAirPressure() {
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP sensor, check wiring!");
    return "ERR";
  }

  int sensorValue = bmp.readPressure();
  String sensorData = String(sensorValue);


  if (isnan(sensorValue) || sensorValue == 0) {
    Serial.println(F("Error reading air pressure!"));
    return "ERR";
  }
  else {
    Serial.print("From BMP180: Pressure = ");
    Serial.print(sensorData);
    Serial.println(" Pa");
  }

  return sensorData;
}

String getInternetTime() {
  // Get time
  // https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
  timeClient.begin();
  delay(500);
  timeClient.update();
  String epochTime = String(timeClient.getEpochTime());
  Serial.print(F("\nCurrent epoch time is:"));
  Serial.println(epochTime); 
  return epochTime;
}

void uploadData(String epochTime, DHT11Data DHT_data, String air_pressure, String other_info) {
  // Uploading
  Serial.println("--- The following should be uploaded ---");
  Serial.print("/update_weather?time=");
  Serial.print(epochTime);
  Serial.print("&temp=");
  Serial.print(DHT_data.temp);
  Serial.print("&air_humidity=");
  Serial.print(DHT_data.air_humidity);
  Serial.print("&air_pressure=");
  Serial.print(air_pressure);
  Serial.print("&other_info=");
  Serial.println(other_info);


  // https://randomnerdtutorials.com/esp8266-nodemcu-http-get-post-arduino/#http-get-1
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String baseUrl = SERVER_ADDR + "/update_weather";
    String url = baseUrl + "?time=" + epochTime + "&temp=" + DHT_data.temp + "&air_humidity=" + DHT_data.air_humidity + \
                                                  "&air_pressure=" + air_pressure + "&other_info=" + other_info;

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