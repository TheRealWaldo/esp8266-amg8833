#include <ESP8266WebServer.h>
#include <SparkFun_GridEYE_Arduino_Library.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFiManager.h>

#define ESP8266_DRD_USE_RTC false
#define ESP_DRD_USE_LITTLEFS true
#define DOUBLERESETDETECTOR_DEBUG false

#include <ESP_DoubleResetDetector.h>

GridEYE grideye;

ESP8266WebServer server(80);
DoubleResetDetector *drd;

String output;

const char *sensor = "AMG8833";
const int size = 8;
const int total_pixels = size * size;

float pixels[total_pixels];

void notFound()
{
    server.send(404, "text/plain", "Not found");
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

void sendRaw()
{
    server.send(200, "application/json", output.c_str());
}

void setup()
{
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);

    WiFiManager wm;

    drd = new DoubleResetDetector(10, 2);

    if (drd->detectDoubleReset())
    {
        Serial.println("Double reset detected");
        wm.resetSettings();
    }

    bool res;
    res = wm.autoConnect("thermalvision");

    if (!res)
    {
        Serial.println("Failed to connect");
        ESP.restart();
    }
    else
    {
        Wire.begin();
        // Library assumes "Wire" for I2C but you can pass something else with begin() if you like
        grideye.begin();

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        server.on("/raw", sendRaw);

        server.onNotFound(notFound);

        server.begin();
        Serial.println("HTTP Server started");
    }
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        getPixels();
        getRaw();
        delay(100);
        server.handleClient();
    }
    drd->loop();
}
