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
    
    typedef enum {
      YY_MODE_NONE,
      YY_MODE_CLIENT,
      YY_MODE_PEER_CLIENT,
      YY_MODE_PEER_SERVER
    } yy_mode_t;
    yy_mode_t currentMode = YY_MODE_NONE;

    typedef enum {
      //compatibility with wl_definitions.h
      YY_NO_SHIELD        = WL_NO_SHIELD,
      YY_IDLE_STATUS      = WL_IDLE_STATUS,
      YY_NO_SSID_AVAIL    = WL_NO_SSID_AVAIL,
      YY_SCAN_COMPLETED   = WL_SCAN_COMPLETED,
      YY_CONNECTED        = WL_CONNECTED,
      YY_CONNECT_FAILED   = WL_CONNECT_FAILED,
      YY_CONNECTION_LOST  = WL_CONNECTION_LOST,
      YY_DISCONNECTED     = WL_DISCONNECTED,
      //yo-yo specific:
      YY_CONNECTED_PEER_CLIENT,
      YY_CONNECTED_PEER_SERVER
    } yy_status_t;
    yy_status_t currentStatus = YY_IDLE_STATUS;

    void onStatusChanged();

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);
    AsyncWebServer webserver = AsyncWebServer(80);

    YoYoWiFiManagerCredentials credentials;
    uint8_t wifiLEDPin;

    typedef bool (*callbackPtr)(const String&, JsonVariant);
    callbackPtr yoYoCommandGetHandler = NULL;
    callbackPtr yoYoCommandPostHandler = NULL;

    void startWebServer();

    bool addAP(String ssid, String pass);

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
    void connect(String ssid, String pass = "");
    bool joinPeerNetwork(char const *apName, char const *apPassword);
    void createPeerNetwork(char const *apName, char const *apPassword);
    bool isConnected();

    bool findNetwork(char const *ssid, char *matchingSSID, bool autocomplete = false, bool autocorrect = false, int autocorrectError = 0);

    int update();

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