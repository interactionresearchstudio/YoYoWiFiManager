#ifndef YoYoNetworkManager_h
#define YoYoNetworkManager_h

#include "Arduino.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiAP.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "Levenshtein.h"

#define SSID_MAX_LENGTH 31
#define WIFICONNECTTIMEOUT 60000

class YoYoNetworkManager
{
  public:
    Preferences preferences;

    String wifiCredentials = "";
    String macCredentials = "";
    String myID = "";
    bool inList;

    uint8_t ledBuiltIn = 2;

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

    WiFiMulti wifiMulti;

    void begin();
    void update();
    void loadCredentials();
    void setPairedStatus();
    int getNumberOfMacAddresses();
    void addToMacAddressJSON(String addr);
    String generateID();
    boolean scanAndConnectToLocalSCADS();
    void createSCADSAP();

  private:
    bool disconnected = false;

    void setupCaptivePortal();
    void setupLocalServer();
    void setupSocketClientEvents();
    void connectToWifi(String credentials);
    void setupSocketIOEvents();
    bool isWifiValid(String incomingSSID);
    String checkSsidForSpelling(String incomingSSID);
};

#endif