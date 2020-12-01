#include "YoYoWiFiManager.h"

YoYoWiFiManager::YoYoWiFiManager() {
}

void YoYoWiFiManager::init(callbackPtr getHandler, callbackPtr postHandler, uint8_t wifiLEDPin) {
  this -> yoYoCommandGetHandler = getHandler;
  this -> yoYoCommandPostHandler = postHandler;

  this -> wifiLEDPin = wifiLEDPin;

  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  SPIFFS.begin();
}

boolean YoYoWiFiManager::begin(char const *apName, char const *apPassword, bool autoconnect) {
  if(autoconnect && credentials.available()) {
    //TODO: attempt to connect using credentials:

    Serial.println("credentials available");
    currentMode = YY_MODE_CLIENT;
    connect();
  }
  else {
    currentMode = createPeerNetwork(apName, apPassword);
  }

  return(true);
}

YoYoWiFiManager::yy_mode_t YoYoWiFiManager::createPeerNetwork(char const *apName, char const *apPassword) {
  yy_mode_t mode = YY_MODE_NONE;
  
  if(joinPeerNetworkAsClient(apName, apPassword)) {
    currentMode = YY_MODE_PEER_CLIENT;
  }
  else {
    currentMode = YY_MODE_PEER_SERVER;
    joinPeerNetworkAsServer(apName, apPassword);
  }

  return(mode);
}

// Scan and connect to peer wifi network. Returns true if joined.
boolean YoYoWiFiManager::joinPeerNetworkAsClient(char const *apName, char const *apPassword) {
  boolean joinedPeerNetwork = false;

  char foundNetwork[SSID_MAX_LENGTH * 2];
  if(findNetwork(apName, foundNetwork, true)) {
    Serial.printf("Found peer network: %s\n", foundNetwork);

    addAP(foundNetwork, apPassword);
    while ((wifiMulti.run() != WL_CONNECTED)) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    startWebServer();
    joinedPeerNetwork = true;
  }

  return (joinedPeerNetwork);
}

// Creates Access Point for other device to connect to
void YoYoWiFiManager::joinPeerNetworkAsServer(char const *apName, char const *apPassword) {
  Serial.print("Wifi name:");
  Serial.println(apName);

  WiFi.mode(WIFI_AP);
  delay(2000);

  WiFi.persistent(false);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apName, apPassword);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  //Start captive portal
  dnsServer.start(DNS_PORT, "*", apIP);

  startWebServer();
}

void YoYoWiFiManager::startWebServer() {
  Serial.println("startWebServer\n");

  webserver.addHandler(this).setFilter(ON_AP_FILTER);  //only when requested from AP
  webserver.begin();
}

void YoYoWiFiManager::connect() {
  int credentialsCount = credentials.getQuantity();
  for(int n = 0; n < credentialsCount; ++n) {
    addAP(credentials.getSSID(n) -> c_str(), credentials.getPassword(n) -> c_str());
  }
}

void YoYoWiFiManager::connect(String ssidAsString, String pass) {
  const char *ssid = ssidAsString.c_str();
  char *ssidChecked = new char[SSID_MAX_LENGTH];
  if(findNetwork(ssid, ssidChecked, false, true, 2)) {
    if(addAP(ssidChecked, pass)) {
      credentials.add(ssidChecked, pass.c_str());
    }
  }

  /*
  while (!connectSuccess) {
    if (millis() - wifiMillis > WIFICONNECTTIMEOUT) {
      // Timeout, check if we're out of range.
      while (hasConnected) {
        // Out of range, keep trying
        uint8_t _currentStatus = wifiMulti.run();
        if (_currentStatus == WL_CONNECTED) {
          digitalWrite(wifiLEDPin, 0);
          preferences.begin("scads", false);
          preferences.putBool("hasConnected", true);
          preferences.end();
          break;
        }
        else {
          digitalWrite(wifiLEDPin, 1);
          delay(100);
          Serial.print(".");
        }
        yield();
      }
    }

    delay(100);
    yield();
    Serial.print(".");
  }
  */
}

bool YoYoWiFiManager::addAP(String ssid, String pass) {
  bool success = false;

  if(ssid.length() > 0 && ssid.length() <= SSID_MAX_LENGTH) {
    success = wifiMulti.addAP(ssid.c_str(), pass.c_str());;
  }

  return(success);
}

bool YoYoWiFiManager::findNetwork(char const *ssid, char *matchingSSID, bool autocomplete, bool autocorrect, int autocorrectError) {
  bool result = false;

  int numberOfNetworks = WiFi.scanNetworks();
  for(int n = 0; n < numberOfNetworks && !result; ++n) {
    bool match = WiFi.SSID(n).equals(ssid) || 
                  (autocomplete && WiFi.SSID(n).startsWith(ssid)) || 
                  (autocorrect && Levenshtein::levenshteinIgnoreCase(ssid, WiFi.SSID(n).c_str()) < autocorrectError);

    if(match) {
      result = true;
      strcpy(matchingSSID, WiFi.SSID(n).c_str());
    }
  }

  return(result);
}

uint8_t YoYoWiFiManager::update() {
  dnsServer.processNextRequest();

  uint8_t wlStatus = (currentMode == YY_MODE_CLIENT) ? wifiMulti.run() : WiFi.status();
  yy_status_t yyStatus = YY_IDLE_STATUS;
  if(wlStatus == WL_CONNECTED) {
    switch(currentMode) {
      case YY_MODE_CLIENT:      yyStatus = YY_CONNECTED; break;
      case YY_MODE_PEER_CLIENT: yyStatus = YY_CONNECTED_PEER_CLIENT; break;
      case YY_MODE_PEER_SERVER: yyStatus = YY_CONNECTED_PEER_SERVER; break;
    }
  }
  else yyStatus = (yy_status_t) wlStatus;  //Otherwise yy_status_t and wl_status_t are value compatible 

  if(currentStatus != yyStatus) {
    currentStatus = yyStatus;
    onStatusChanged();
  }

  digitalWrite(wifiLEDPin, currentStatus != YY_CONNECTED);

  return(currentStatus); 
}

void YoYoWiFiManager::onStatusChanged() {
  switch(currentStatus) {
    case YY_CONNECTED:
      blinkWiFiLED(3);

      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case YY_CONNECTED_PEER_CLIENT:
      break;
    case YY_CONNECTED_PEER_SERVER:
      break;
    case YY_NO_SHIELD:
      break;
    case YY_IDLE_STATUS:
      break;
    case YY_NO_SSID_AVAIL:
      break;
    case YY_SCAN_COMPLETED:
      break;
    case YY_CONNECT_FAILED:
      break;
    case YY_CONNECTION_LOST:
      break;
    case YY_DISCONNECTED:
      break;
  }
}

void YoYoWiFiManager::blinkWiFiLED(int count) {
  for (byte i = 0; i < count; i++) {
    digitalWrite(wifiLEDPin, 1);
    delay(100);
    digitalWrite(wifiLEDPin, 0);
    delay(400);
  }
  delay(600);
}

//AsyncWebHandler
//===============

bool YoYoWiFiManager::canHandle(AsyncWebServerRequest *request) {
  //we can handle anything!
  return true;
}

void YoYoWiFiManager::handleRequest(AsyncWebServerRequest *request) {
  Serial.print("handleRequest: ");
  Serial.println(request->url());

  if (request->method() == HTTP_GET) {
    if(request->url().startsWith("/yoyo")) {
      if (request->url() == "/yoyo/networks")    getNetworks(request);
      else if (request->url() == "/yoyo/peers")  getPeers(request);
      else if (request->url() == "/yoyo/credentials")  getCredentials(request);
      else onYoYoCommandGET(request);
    }
    else if (SPIFFS.exists(request->url())) {
      sendFile(request, request->url());
    }
    else if (request->url().endsWith(".html") || 
              request->url().endsWith("/") ||
              request->url().endsWith("generate_204") ||
              request->url().endsWith("redirect"))  {
      sendFile(request, "/index.html");
    }
    else if (request->url().endsWith("connecttest.txt") || 
              request->url().endsWith("ncsi.txt")) {
      request->send(200, "text/plain", "Microsoft NCSI");
    }
    else if (strstr(request->url().c_str(), "generate_204_") != NULL) {
      Serial.println("you must be huawei!");
      sendFile(request, "/index.html");
    }
    else {
      request->send(304);
    }
  }
  else if (request->method() == HTTP_POST) {
    request->send(400); //POSTs are expected to have a body and then be processes by handleBody()
  }
  else {
    request->send(400);
  }
}

void YoYoWiFiManager::handleBody(AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.print("handleBody: ");
  Serial.println(request->url());

  if (request->method() == HTTP_GET) {
    request->send(400); //GETs are expected to have no body and then be processes by handleRequest()
  }
  else if (request->method() == HTTP_POST) {
    if(request->url().startsWith("/yoyo")) {
      String json = "";
      for (int i = 0; i < len; i++)  json += char(data[i]);

      StaticJsonDocument<1024> jsonDoc;
      if (!deserializeJson(jsonDoc, json)) {
        if (request->url() == "/yoyo/credentials")  setCredentials(request, jsonDoc.as<JsonVariant>());
        else {
          onYoYoCommandPOST(request, jsonDoc.as<JsonVariant>());
        }
      }
    }
    else {
      request->send(404);
    }
  }
  else {
    request->send(400);
  }
}

void YoYoWiFiManager::sendFile(AsyncWebServerRequest * request, String path) {
  Serial.println("handleFileRead: " + path);

  if (SPIFFS.exists(path)) {
    request->send(SPIFFS, path, getContentType(path));
  }
  else {
    request->send(404);
  }
}

String YoYoWiFiManager::getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

void YoYoWiFiManager::onYoYoCommandGET(AsyncWebServerRequest *request) {
  bool success = false;

  AsyncResponseStream *response = request->beginResponseStream("application/json");

  StaticJsonDocument<1024> settingsJsonDoc;
  if(yoYoCommandGetHandler) {
    success = yoYoCommandGetHandler(request->url(), settingsJsonDoc.as<JsonVariant>());
  }

  if(success) {
    String jsonString;
    serializeJson(settingsJsonDoc, jsonString);
    response->print(jsonString);

    request->send(response);
  }
  else {
    request->send(400);
  }
}

void YoYoWiFiManager::onYoYoCommandPOST(AsyncWebServerRequest *request, JsonVariant json) {
  bool success = false;

  if(yoYoCommandPostHandler) {
    success = yoYoCommandPostHandler(request->url(), json);
  }

  request->send(success ? 200 : 404);
}

void YoYoWiFiManager::getCredentials(AsyncWebServerRequest *request) {
  request->send(200);
}

void YoYoWiFiManager::setCredentials(AsyncWebServerRequest *request, JsonVariant json) {
  request->send(200);
}

void YoYoWiFiManager::getPeers(AsyncWebServerRequest * request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  response->print(getPeersAsJsonString());
  request->send(response);
}

String YoYoWiFiManager::getPeersAsJsonString() {
  String jsonString;

  StaticJsonDocument<1000> jsonDoc;
  getPeersAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getPeersAsJson(JsonDocument& jsonDoc) {
  //TODO: if client of AP
  JsonArray peers = jsonDoc.createNestedArray();
 
  int peerCount = updatePeerList();

  char *ipAddress = new char[17];
  char *macAddress = new char[18];
  for (int i = 0; i < peerCount; i++) {
    getPeerN(i, ipAddress, macAddress, true);
 
    JsonObject peer  = peers.createNestedObject();
    peer["IP"] = ipAddress;
    peer["MAC"] = macAddress;   
  }
  delete ipAddress;
  delete macAddress;
}

int YoYoWiFiManager::updatePeerList() {
  int count = 0;

  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
  count = adapter_sta_list.num;

  return(count);
}

bool YoYoWiFiManager::getPeerN(int n, char *ipAddress, char *macAddress, bool unchecked) {
  bool success = false;

  if(unchecked || (n >=0 && n < updatePeerList())) {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[n];

    strcpy(ipAddress, ip4addr_ntoa(&(station.ip)));
    mac_addr_to_c_str(station.mac, macAddress);

    success = true;
  }

  return(success);
}

void YoYoWiFiManager::getNetworks(AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    response->print(getNetworksAsJsonString());
    request->send(response);
}

String YoYoWiFiManager::getNetworksAsJsonString() {
  String jsonString;

  StaticJsonDocument<1000> jsonDoc;
  getNetworksAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getNetworksAsJson(JsonDocument& jsonDoc) {
  JsonArray networks = jsonDoc.createNestedArray();

  int n = WiFi.scanNetworks();
  n = (n > MAX_NETWORKS_TO_SCAN) ? MAX_NETWORKS_TO_SCAN : n;

  //Array is ordered by signal strength - strongest first
  for (int i = 0; i < n; ++i) {
    String networkSSID = WiFi.SSID(i);
    if (networkSSID.length() <= SSID_MAX_LENGTH) {
      JsonObject network  = networks.createNestedObject();
      network["SSID"] = WiFi.SSID(i);
      network["BSSID"] = WiFi.BSSIDstr(i);
      network["RSSI"] = WiFi.RSSI(i);
    }
  }
}

bool YoYoWiFiManager::mac_addr_to_c_str(uint8_t *mac, char *str) {
  bool success = true;

  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return(success);
}