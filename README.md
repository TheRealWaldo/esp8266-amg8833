# ESP89266 AMG8833

This repository is a work in progress.  Source code for creating firmware for an ESP8266 chip (d1 mini in my case) to use an AMG8833 sensor with https://github.com/TheRealWaldo/thermal

## Secrets

You'll need to create a `secrets.h` file in `include/` which looks like this:

```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
```

But obviously replace the SSID and password with your own.
