#ifndef YoYoWiFiManager_h
#define YoYoWiFiManager_h

#include "Arduino.h"

#include <ArduinoJson.h>
#include <DNSServer.h>

#if defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
  #include <ESPAsyncTCP.h>      //not currently available via Library Manager > https://github.com/me-no-dev/ESPAsyncTCP
  #include <ESP8266HTTPClient.h>
  #include <FS.h>
  #include "YoYoWiFiManager/wifi_sta.h"

#elif defined(ESP32)
  #include <HTTPClient.h>
  #include <HTTPUpdate.h>
  #include <WiFiMulti.h>
  #include <esp_wifi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#endif
#include <ESPAsyncWebServer.h>

#include "YoYoWiFiManager/YoYoNetworkSettingsInterface.h"
#include "YoYoWiFiManager/Levenshtein.h"
#include "YoYoWiFiManager/Espressif.h"
#include "YoYoWiFiManager/index_html.h"

#define SSID_MAX_LENGTH 31
#define WIFICLIENTTIMEOUT 20000
#define WIFISERVERTIMEOUT 60000
#define MAX_SYNC_DELAY 3000
#define SCAN_NETWORKS_MIN_INT 30000

typedef enum {
  //compatibility with wl_status_t (wl_definitions.h)
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

class YoYoWiFiManager : public AsyncWebHandler {
  public:
  typedef enum {
    YY_MODE_NONE,
    YY_MODE_CLIENT,
    YY_MODE_PEER_CLIENT,
    YY_MODE_PEER_SERVER
  } yy_mode_t;
  yy_mode_t currentMode = YY_MODE_NONE;

  private:
    bool running = false;

    #if defined(ESP8266)
      ESP8266WiFiMulti wifiMulti;
    #elif defined(ESP32)
      WiFiMulti wifiMulti;
    #endif

    char peerNetworkSSID[SSID_MAX_LENGTH + 1];
    char peerNetworkPassword[64];
    
    yy_status_t currentStatus = YY_IDLE_STATUS;

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);

    int webServerPort = 80;
    AsyncWebServer *webserver = NULL;
    bool startWebServerOnceConnected = false;

    uint32_t clientTimeOutAtMs = 0;
    void updateClientTimeOut();
    bool clientHasTimedOut();

    uint32_t serverTimeOutAtMs = 0;
    void updateServerTimeOut();
    bool serverHasTimedOut();

    yy_mode_t updateTimeOuts();

    uint32_t lastScanNetworksAtMs = 0;

    YoYoNetworkSettingsInterface *settings = NULL;
    uint8_t wifiLEDPin;

    bool SPIFFS_ENABLED = false;

    typedef void (*voidCallbackPtr)();
    voidCallbackPtr onYY_CONNECTEDhandler = NULL;

    typedef bool (*jsonCallbackPtr)(const String&, JsonVariant);
    jsonCallbackPtr yoYoCommandGetHandler = NULL;
    jsonCallbackPtr yoYoCommandPostHandler = NULL;

    String rootIndexFile = "/index.html";

    void startWebServer();
    void stopWebServer();

    void addPeerNetwork(char *ssid, char *password);
    void addKnownNetworks();
    bool addNetwork(char const *ssid, char const *password, bool autosave = true);

    void startPeerNetworkAsAP();
    void stopPeerNetworkAsAP();

    String getCredentialsAsJsonString();
    void getCredentialsAsJson(JsonDocument& jsonDoc);

    int scanNetworks();
    String getNetworksAsJsonString();
    void getNetworksAsJson(JsonDocument& jsonDoc);

    String getClientsAsJsonString();
    void getClientsAsJson(JsonDocument& jsonDoc);

    String getPeersAsJsonString();
    void getPeersAsJson(JsonDocument& jsonDoc);
    void createNestedPeer(JsonDocument& jsonDoc, IPAddress *ip, uint8_t *macAddress, bool localhost = false, bool gateway = false);
    int updateClientList();
    bool getPeerN(int n, IPAddress *ipAddress, uint8_t *macAddress);

    #if defined(ESP32)
      wifi_sta_list_t wifi_sta_list;
    #endif
    tcpip_adapter_sta_list_t adapter_sta_list;

    int POST(const char *server, const char *path, JsonVariant json);
    int POST(const char *server, const char *path, const char *payload);
    int GET(const char *server, const char *path, JsonDocument &json);
    int GET(const char *server, const char *path, char *payload);

    bool setMode(yy_mode_t mode);

    void printModeAndStatus();
  public:
    YoYoWiFiManager();

    void init(YoYoNetworkSettingsInterface *settings = NULL, voidCallbackPtr onYY_CONNECTEDhandler = NULL, jsonCallbackPtr getHandler = NULL, jsonCallbackPtr postHandler = NULL, bool startWebServerOnceConnected = false, int webServerPort = 80, uint8_t wifiLEDPin = 2);
    boolean begin(char const *apName, char const *apPassword = NULL, bool autoconnect = true);
    void end();
    void connect();

    uint8_t loop();
    yy_status_t getStatus();

    void blinkWiFiLED(int count);
    bool findNetwork(char const *ssid, char *matchingSSID, bool autocomplete = false, bool autocorrect = false, int autocorrectError = 0);

    //AsyncWebHandler:
    bool canHandle(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    void handleCaptivePortalRequest(AsyncWebServerRequest *request);
    void handleBody(AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total);
    void sendFile(AsyncWebServerRequest * request, String path);
    void sendIndexFile(AsyncWebServerRequest * request);
    String getContentType(String filename);

    void onYoYoCommandGET(AsyncWebServerRequest *request);
    void onYoYoCommandPOST(AsyncWebServerRequest *request, JsonVariant json);
    bool broadcastToPeersPOST(String path, JsonVariant json);

    void getNetworks(AsyncWebServerRequest * request);
    void getClients(AsyncWebServerRequest * request);
    void getPeers(AsyncWebServerRequest * request);
    void getCredentials(AsyncWebServerRequest *request);
    
    bool setCredentials(AsyncWebServerRequest *request, JsonVariant json);
    bool setCredentials(JsonVariant json);

    bool hasPeers();
    int countPeers();
    bool hasClients();
    int countClients();

    bool isEspressif(uint8_t *macAddress);
    void setRootIndexFile(String rootIndexFile);

  private:
    bool mac_addr_to_c_str(uint8_t *mac, char *str);
    int getOUI(char *mac);
    int getOUI(uint8_t *mac);
    int getOUI(uint8_t a, uint8_t b, uint8_t c, uint8_t d = 0, uint8_t e = 0, uint8_t f = 0);
};

#endif