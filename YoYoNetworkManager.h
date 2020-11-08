#ifndef YoYoNetworkManager_h
#define YoYoNetworkManager_h

#include "Arduino.h"

#include <Preferences.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "CaptiveRequestHandler.h"
#include "YoYoWifi.h"

class YoYoNetworkManager
{
  private:
    Preferences preferences;

    String wifiCredentials = "";
    String macCredentials = "";
    String myID = "";
    bool inList;

    uint8_t wifiLEDPin;

    enum PAIRED_STATUS {
      remoteSetup,
      localSetup,
      pairedSetup
    };
    int currentPairedStatus = remoteSetup;

    enum SETUP_STATUS {
      setup_pending,
      setup_client,
      setup_server,
      setup_finished
    };
    int currentSetupStatus = setup_pending;

    //Access Point credentials
    String scads_ssid = "";
    String scads_pass = "blinkblink";

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);

    YoYoWifi yoyoWifi;

    bool disconnected = false;

    bool isResetting = false;
    unsigned long resetTime;
    int resetDurationMs = 4000;

    void loadCredentials();
    void setPairedStatus();
    int getNumberOfMacAddresses();
    void addToMacAddressJSON(String addr);
    String generateID();

    void setupCaptivePortal();
    void setupLocalServer();
    void setupSocketClientEvents();
    void setupSocketIOEvents();

  public:
    void begin(uint8_t wifiLEDPin = 2);
    void update();

    void printWifiStatus(uint8_t status);

    void factoryReset();
    void softReset(int delayMs);
    void checkReset();
};

#endif
