
/**
 * ESP8266 temp/humidity/pressure IOT device for io.adafruit.com
 * Repo: https://github.com/onebytegone/iot_esp8266_bme280_adafruitio
 * Author: Ethan Smith <ethan@onebytegone.com>
 *
 * NOTE: Please see `config.h` for io.adafruit.com and WiFi config
 */

#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
   Serial.begin(115200);
   Serial.println();

   // Turn the LED on by making the voltage LOW
   pinMode(LED_BUILTIN, OUTPUT);
   digitalWrite(LED_BUILTIN, LOW);

   if (!bme.begin(BME280_I2C_ADDRESS)) {
      Serial.println("BME280 sensor was not found.");
      // Blink LED rapidly to indicate an error
      int state = LOW;
      while (1) {
         digitalWrite(LED_BUILTIN, state);
         state = !state;
         delay(100);
      }
   }

   Serial.print("connecting to ");
   Serial.println(NETWORK_SSID);
   WiFi.mode(WIFI_STA);
   WiFi.begin(NETWORK_SSID, NETWORK_PSK);
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());

   // Once connected to WiFi, turn the LED off
   digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
   double humidity = bme.readHumidity(); // percent
   double temperature = (bme.readTemperature() * 9 / 5) + 32; // farenheit
   double pressure = bme.readPressure() / 100.0; // millbar

   transmitValue(IO_HUMIDITY_FEED, String(humidity));
   transmitValue(IO_TEMPERATURE_FEED, String(temperature));
   transmitValue(IO_PRESSURE_FEED, String(pressure));

   delay((60 * 1000) / REPORTS_PER_MIN);
}

void transmitValue(String feed, String value) {
   String path = "";
   BearSSL::WiFiClientSecure secureClient;
   HTTPClient https;
   int didBeginSuccessfully, httpStatusCode;

   path += "/api/v2/";
   path += IO_USERNAME;
   path += "/feeds/";
   path += feed;
   path += "/data";

   secureClient.setInsecure();
   didBeginSuccessfully = https.begin(secureClient, "io.adafruit.com", 443, path, true);

   if (!didBeginSuccessfully) {
      Serial.print("Failed to begin connection to ");
      Serial.print(path);
      return;
   }

   https.addHeader("Content-Type", "application/x-www-form-urlencoded");
   https.addHeader("X-AIO-Key", IO_KEY);

   httpStatusCode = https.POST("value=" + value);
   Serial.print("POST status: ");
   Serial.print(httpStatusCode);
   Serial.print(" (");
   Serial.print(feed);
   Serial.print(", ");
   Serial.print(value);
   Serial.println(")");

   if (httpStatusCode != 200) {
      Serial.print("Did not get 200 response from ");
      Serial.println(path);
      Serial.println(https.getString());
   }

   https.end();
   secureClient.stop();
}
