#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SparkFun_GridEYE_Arduino_Library.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <secrets.h>

GridEYE grideye;

AsyncWebServer server(80);

String output;

const char *sensor = "AMG8833";
const int size = 8;
const int total_pixels = size * size;

float pixels[total_pixels];

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void getPixels()
{
    for (unsigned char i = 0; i < total_pixels; i++)
    {
        pixels[i] = grideye.getPixelTemperature(i);
    }
}

void getRaw()
{
    String payload;
    String new_output;

    StaticJsonDocument<512> doc;

    for (unsigned char i = 0; i < total_pixels; i++)
    {
        payload = payload + pixels[i];
        if (i < total_pixels - 1)
        {
            payload = payload + ",";
        }
    }

    doc["sensor"] = sensor;
    doc["rows"] = size;
    doc["cols"] = size;
    doc["data"] = payload.c_str();
    doc["temp"] = grideye.getDeviceTemperature();

    serializeJson(doc, new_output);
    output = new_output;
}

void setup()
{
    Serial.begin(9600);

    Wire.begin();
    // Library assumes "Wire" for I2C but you can pass something else with begin() if you like
    grideye.begin();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/raw", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "application/json", output.c_str()); });

    server.onNotFound(notFound);

    server.begin();
}

void loop()
{
    getPixels();
    getRaw();
    delay(100);
}
