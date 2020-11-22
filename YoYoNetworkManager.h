#ifndef YoYoNetworkManager_h
#define YoYoNetworkManager_h

#include "Arduino.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "CaptiveRequestHandler.h"

#include "YoYoWifi.h"
#include "YoYoWsClient.h"
#include "YoYoNetworkManagerPreferences.h"

#include "Levenshtein.h"

class YoYoNetworkManager
{
  private:
    YoYoNetworkManagerPreferences preferences;

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
    static int currentPairedStatus;

    enum SETUP_STATUS {
      setup_pending,
      setup_client,
      setup_server,
      setup_finished
    };
    static int currentSetupStatus;

    //Access Point credentials
    String scads_ssid = "";
    String scads_pass = "blinkblink";

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);

    YoYoWifi yoyoWifi;
    static YoYoWsClient wsClient;

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
    void setupSocketIOEvents();
    static void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
    static void decodeWsData(const char* data);

  public:
    void begin(uint8_t wifiLEDPin = 2);
    void update();

    void printWifiStatus(uint8_t status);

    void factoryReset();
    void softReset(int delayMs);
    void checkReset();
};

#endif
