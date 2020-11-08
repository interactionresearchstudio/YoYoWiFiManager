#ifndef YoYoWifi_h
#define YoYoWifi_h

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiAP.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "Levenshtein.h"

#define SSID_MAX_LENGTH 31
#define WIFICONNECTTIMEOUT 60000

class YoYoWifi
{
  private:
    uint8_t wifiLEDPin;
    bool disconnected = true;
    uint32_t wificheckMillis;
    uint32_t wifiCheckTime = 5000;

    //Access Point credentials
    String scads_ssid = "";
    String scads_pass = "blinkblink";

    IPAddress apIP = IPAddress(192, 168, 4, 1);

    Preferences preferences;
    WiFiMulti wifiMulti;

    String checkSsidForSpelling(String incomingSSID);
    bool isWifiValid(String incomingSSID);
    void printWifiStatus(uint8_t status);

  public:
    void wifiCheck();
    void connectToWifi(String credentials);
    bool scanAndConnectToLocalSCADS();
    void createSCADSAP();
    bool isConnected();
    void attachLed(uint8_t pin);
    String generateID();
};

#endif
