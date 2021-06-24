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
const int nearest_neighbour_size = (size * 2) - 1;
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
    output = "";
    StaticJsonDocument<512> doc;

    getPixels();
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

    serializeJson(doc, output);
}

void getNearestNeighbor()
{
    unsigned int i = 0;
    unsigned int j = 0;
    float nearestNeighbor[nearest_neighbour_size][nearest_neighbour_size];

    StaticJsonDocument<2048> doc;
    String payload;

    getPixels();
    output = "";

    doc["sensor"] = sensor;
    doc["rows"] = nearest_neighbour_size;
    doc["cols"] = nearest_neighbour_size;

    // Load the 8x8 data into the target matrix.
    // Data from even numbered rows are loaded into positions 0,2,4,6,8,10,12,14
    // Odd numbered rows remain empty.
    for (unsigned int r = 0; r < size; r++)
    {
        for (unsigned int c = 0; c < size; c++)
        {
            nearestNeighbor[(r + j)][(c + i)] = pixels[(r * size) + c];
            ++i;
        }
        i = 0;
        j++;
    }

    // Fill in the blanks as the average of the nearest neighbours.
    // e_map will hold the expanded matrix.  matrix holds the serialized data as a string.

    for (unsigned int r = 0; r < nearest_neighbour_size; r++)
    {
        for (unsigned int c = 0; c < nearest_neighbour_size; c++)
        {
            if ((c % 2 != 0) && (c < (nearest_neighbour_size - 1)))
            {
                nearestNeighbor[r][c] = (float)((nearestNeighbor[r][c - 1] + nearestNeighbor[r][c + 1]) / 2);
                payload = payload + nearestNeighbor[r][c] + ",";
            }
            if ((r % 2 != 0) && (r < (nearest_neighbour_size - 1)))
            {
                nearestNeighbor[r][c] = (float)((nearestNeighbor[r - 1][c] + nearestNeighbor[r + 1][c]) / 2);
                payload = payload + nearestNeighbor[r][c] + ",";
            }
        }
    }

    doc["data"] = payload;

    serializeJson(doc, output);
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
              {
                  getRaw();
                  request->send(200, "application/json", output.c_str());
              });

    server.on("/neighbor", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  getNearestNeighbor();
                  request->send(200, "application/json", output.c_str());
              });

    server.onNotFound(notFound);

    server.begin();
}

void loop()
{
    // toss in a delay because we don't need to run all out
    //delay(100);
}
