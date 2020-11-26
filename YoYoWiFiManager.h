#ifndef YoYoWiFiManager_h
#define YoYoWiFiManager_h

#include "Arduino.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiMulti.h>

#include "esp_wifi.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

#include "YoYoWiFiManagerCredentials.h"

#include "Levenshtein.h"

#define SSID_MAX_LENGTH 31
#define WIFICONNECTTIMEOUT 60000
#define MAX_NETWORKS_TO_SCAN 5

class YoYoWiFiManager : public AsyncWebHandler {
  private:
    WiFiMulti wifiMulti;

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);
    AsyncWebServer webserver = AsyncWebServer(80);

    YoYoWiFiManagerCredentials credentials;

    String wifiCredentials = "";
    String macCredentials = "";
    String myID = "";
    bool inList;

    uint8_t wifiLEDPin;

    typedef bool (*callbackPtr)(const String&, JsonVariant);
    callbackPtr yoYoCommandGetHandler = NULL;
    callbackPtr yoYoCommandPostHandler = NULL;

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

    bool disconnected = false;

    int getNumberOfMacAddresses();
    void addToMacAddressJSON(String addr);

    void startWebServer();

    //-----------------------------
    uint32_t wificheckMillis;
    uint32_t wifiCheckTime = 5000;


    bool isSSIDValid(char const *ssid);
    void printWifiStatus(uint8_t status);

    String getNetworksAsJsonString();
    void getNetworksAsJson(JsonDocument& jsonDoc);

    String getPeersAsJsonString();
    void getPeersAsJson(JsonDocument& jsonDoc);

    int updatePeerList();
    bool getPeerN(int n, char *ipAddress, char *macAddress, bool unchecked = false);

    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;

  public:
    YoYoWiFiManager(callbackPtr getHandler = NULL, callbackPtr postHandler = NULL, uint8_t wifiLEDPin = 2);

    boolean begin(char const *apName, char const *apPassword = NULL, bool autoconnect = false);

    void wifiCheck();
    void connect();
    bool joinPeerNetwork(char const *apName, char const *apPassword);
    void createPeerNetwork(char const *apName, char const *apPassword);
    bool isConnected();

    bool findNetwork(char const *ssid, char *matchingSSID, bool autocomplete = false, bool autocorrect = false, int autocorrectError = 0);

    void update();

    //AsyncWebHandler:
    bool canHandle(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    void handleBody(AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total);
    void sendFile(AsyncWebServerRequest * request, String path);
    String getContentType(String filename);

    void onYoYoCommandGET(AsyncWebServerRequest *request);
    void onYoYoCommandPOST(AsyncWebServerRequest *request, JsonVariant json);

    void getNetworks(AsyncWebServerRequest * request);
    void getPeers(AsyncWebServerRequest * request);

  private:
    bool mac_addr_to_c_str(uint8_t *mac, char *str);
};

#endif