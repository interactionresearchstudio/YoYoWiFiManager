#ifndef YoYoWiFiManager_h
#define YoYoWiFiManager_h

#include "Arduino.h"

#include <ArduinoJson.h>
#include <DNSServer.h>

#if defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
  #include <ESPAsyncTCP.h>      //not currently available via Library Manager > https://github.com/me-no-dev/ESPAsyncTCP
  #include <FS.h>
#elif defined(ESP32)
  #include <HTTPClient.h>
  #include <HTTPUpdate.h>
  #include <WiFiMulti.h>
  #include <esp_wifi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#endif
#include <ESPAsyncWebServer.h>

#include "YoYoWiFiManagerSettings.h"
#include "Levenshtein.h"
#include "Espressif.h"
#include "index_html.h"

#define SSID_MAX_LENGTH 31
#define WIFICONNECTTIMEOUT 60000
#define MAX_NETWORKS_TO_SCAN 5

class YoYoWiFiManager : public AsyncWebHandler {
  public:
  typedef enum {
    YY_MODE_NONE,
    YY_MODE_CLIENT,
    YY_MODE_PEER_CLIENT,
    YY_MODE_PEER_SERVER
  } yy_mode_t;
  yy_mode_t currentMode = YY_MODE_NONE;

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

  private:
    #if defined(ESP8266)
      ESP8266WiFiMulti wifiMulti;
    #elif defined(ESP32)
      WiFiMulti wifiMulti;
    #endif

    char peerNetworkSSID[SSID_MAX_LENGTH + 1];
    char peerNetworkPassword[64];
    
    yy_status_t currentStatus = YY_IDLE_STATUS;
    void onStatusChanged();

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(192, 168, 4, 1);

    int webServerPort = 80;
    AsyncWebServer *webserver = NULL;
    bool startWebServerOnceConnected = false;

    long clientTimeOutAtMs = -1;

    YoYoWiFiManagerSettings *settings = NULL;
    uint8_t wifiLEDPin;

    bool SPIFFS_ENABLED = false;

    typedef bool (*callbackPtr)(const String&, JsonVariant);
    callbackPtr yoYoCommandGetHandler = NULL;
    callbackPtr yoYoCommandPostHandler = NULL;

    void startWebServer();
    void stopWebServer();

    yy_mode_t updateMode();
    uint8_t updateStatus();

    void addKnownNetworks();
    bool addNetwork(char const *ssid, char const *password, bool autosave = true);

    yy_mode_t createPeerNetwork();
    bool joinPeerNetworkAsClient();
    void joinPeerNetworkAsServer();

    String getNetworksAsJsonString();
    void getNetworksAsJson(JsonDocument& jsonDoc);

    String getClientsAsJsonString();
    void getClientsAsJson(JsonDocument& jsonDoc);

    String getPeersAsJsonString();
    void getPeersAsJson(JsonDocument& jsonDoc);

    int updateClientList();
    bool getPeerN(int n, char *ipAddress, char *macAddress);

    #if defined(ESP8266)
    #elif defined(ESP32)
      wifi_sta_list_t wifi_sta_list;
      tcpip_adapter_sta_list_t adapter_sta_list;
    #endif

    void makePOST(const char *server, const char *path, JsonVariant json);

    bool setMode(yy_mode_t mode);
    void updateClientTimeOut();

  public:
    YoYoWiFiManager();

    void init(YoYoWiFiManagerSettings *settings, callbackPtr getHandler = NULL, callbackPtr postHandler = NULL, bool startWebServerOnceConnected = false, int webServerPort = 80, uint8_t wifiLEDPin = 2);
    boolean begin(char const *apName, char const *apPassword = NULL, bool autoconnect = false);
    void setPeerNetworkCredentials(char *ssid, char *password);
    void connect();

    void blinkWiFiLED(int count);

    bool findNetwork(char const *ssid, char *matchingSSID, bool autocomplete = false, bool autocorrect = false, int autocorrectError = 0);

    uint8_t update();

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

    void getNetworks(AsyncWebServerRequest * request);
    void getClients(AsyncWebServerRequest * request);
    void getPeers(AsyncWebServerRequest * request);

    int countPeers();
    bool hasClients();
    int countClients();

    void getCredentials(AsyncWebServerRequest *request);
    void setCredentials(AsyncWebServerRequest *request, JsonVariant json);
    bool setCredentials(JsonVariant json);

    bool isEspressif(char *macAddress);
    
  private:
    bool mac_addr_to_c_str(uint8_t *mac, char *str);
    int getOUI(char *mac);
};

#endif