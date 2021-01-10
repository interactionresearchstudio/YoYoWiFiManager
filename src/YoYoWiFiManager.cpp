#include "YoYoWiFiManager.h"

YoYoWiFiManager::YoYoWiFiManager() {
}

void YoYoWiFiManager::init(YoYoNetworkSettingsInterface *settings, voidCallbackPtr onYY_CONNECTEDhandler, jsonCallbackPtr getHandler, jsonCallbackPtr postHandler, bool startWebServerOnceConnected, int webServerPort, uint8_t wifiLEDPin) {
  this -> settings = settings;
  this -> onYY_CONNECTEDhandler = onYY_CONNECTEDhandler;
  this -> yoYoCommandGetHandler = getHandler;
  this -> yoYoCommandPostHandler = postHandler;

  this -> startWebServerOnceConnected = startWebServerOnceConnected;
  this -> webServerPort = webServerPort;

  this -> wifiLEDPin = wifiLEDPin;
  pinMode(wifiLEDPin, OUTPUT);

  #if defined(ESP32)
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  #endif
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  SPIFFS_ENABLED = SPIFFS.begin();

  peerNetworkSSID[0] = NULL;
  peerNetworkPassword[0] = NULL;
}

boolean YoYoWiFiManager::begin(char const *apName, char const *apPassword, bool autoconnect) {
  running = true;

  addPeerNetwork((char *)apName, (char *)apPassword);
  wifiMulti.run();  //prioritise joining peer networks over known networks

  if(autoconnect && settings && settings -> hasNetworkCredentials()) {
    Serial.println("network credentials available");
    addKnownNetworks();
    setMode(YY_MODE_CLIENT);
  }
  else {
    setMode(YY_MODE_PEER_CLIENT); //attempt to join peer network;
  }

  return(true);
}

void YoYoWiFiManager::end() {
  running = false;
}

void YoYoWiFiManager::connect() {
  //Once in YY_MODE_CLIENT mode - loop() will trigger wifiMulti.run()
  setMode(YY_MODE_CLIENT);
}

// Creates Access Point for other device to connect to
void YoYoWiFiManager::startPeerNetworkAsAP() {
  Serial.print("Wifi name:");
  Serial.println(peerNetworkSSID);

  WiFi.mode(WIFI_AP);
  delay(2000);

  WiFi.persistent(false);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(peerNetworkSSID, peerNetworkPassword);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  //Start captive portal
  dnsServer.start(DNS_PORT, "*", apIP);
}

void YoYoWiFiManager::startWebServer() {
  Serial.println("startWebServer\n");

  if(webserver == NULL) {
    webserver = new AsyncWebServer(webServerPort);
    webserver -> addHandler(this);
    webserver -> begin();
  }
}

void YoYoWiFiManager::stopWebServer() {
  if(webserver != NULL) {
    webserver -> end();
    delete(webserver);
    webserver = NULL;
  }
}

void YoYoWiFiManager::addPeerNetwork(char *ssid, char *password) {
  if(ssid != NULL) {
    strcpy(peerNetworkSSID, ssid);
    if(password != NULL) strcpy(peerNetworkPassword, password);

    addNetwork(peerNetworkSSID, peerNetworkPassword, false);
  }
}

void YoYoWiFiManager::addKnownNetworks() {
  if(settings) {
    int numberOfNetworkCredentials = settings->getNumberOfNetworkCredentials();
    char *ssid = new char[32];
    char *password = new char[64];
    for(int n = 0; n < numberOfNetworkCredentials; ++n) {
      settings -> getSSID(n, ssid);
      settings -> getPassword(n, password);
      addNetwork(ssid, password, false);
    }
    delete ssid, password;
  }
}

bool YoYoWiFiManager::addNetwork(char const *ssid, char const *password, bool save) {
  Serial.printf("YoYoWiFiManager::addNetwork %s  %s\n", ssid, password);

  bool success = false;

  if(strlen(ssid) > 0 && strlen(ssid) <= SSID_MAX_LENGTH) {
    char *matchingSSID = new char[SSID_MAX_LENGTH];

    if(findNetwork(ssid, matchingSSID, false, true, 2)) {
      ssid = matchingSSID;
    }

    if(wifiMulti.addAP(ssid, password)) {
      if(save && settings) settings -> addNetwork(ssid, password);
      success = true;
    }
    delete matchingSSID;
  }

  return(success);
}

bool YoYoWiFiManager::findNetwork(char const *ssid, char *matchingSSID, bool autocomplete, bool autocorrect, int autocorrectError) {
  bool result = false;

  int numberOfNetworks = scanNetworks();
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

yy_status_t YoYoWiFiManager::getStatus() {
  uint8_t wlStatus = (currentMode == YY_MODE_PEER_SERVER) ?  WiFi.status() : wifiMulti.run();
  yy_status_t yyStatus = currentStatus;

  if(wlStatus == WL_CONNECTED) {
    switch(currentMode) {
      case YY_MODE_CLIENT:      yyStatus = YY_CONNECTED; break;
      case YY_MODE_PEER_CLIENT: yyStatus = YY_CONNECTED_PEER_CLIENT; break;
    }
  }
  else if(currentMode == YY_MODE_PEER_SERVER && hasClients()) {
    yyStatus = YY_CONNECTED_PEER_SERVER;
  }
  else {
    yyStatus = (yy_status_t) wlStatus;  //Otherwise yy_status_t and wl_status_t are value compatible
  }

  return(yyStatus);
}

uint8_t YoYoWiFiManager::loop() {
  yy_status_t yyStatus = (yy_status_t) WiFi.status();

  if(running) {
    yyStatus = getStatus();

    //Only when the status changes:
    if(currentStatus != yyStatus) {
      switch(yyStatus) {
        //implicitly in YY_MODE_CLIENT
        case YY_CONNECTED:
          if(WiFi.SSID().equals(peerNetworkSSID)) {
            setMode(YY_MODE_PEER_CLIENT);
          }
          else {
            blinkWiFiLED(3);
            Serial.printf("Connected to: %s\n", WiFi.SSID().c_str());
            Serial.println(WiFi.localIP());
          }
          if(onYY_CONNECTEDhandler) {
            onYY_CONNECTEDhandler();
          }
        break;
        //implicitly in YY_MODE_PEER_CLIENT
        case YY_CONNECTED_PEER_CLIENT:
          Serial.printf("Connected to Peer Network: %s\n", WiFi.SSID().c_str());
          Serial.println(WiFi.localIP());
        break;
        //implicitly in YY_MODE_PEER_SERVER
        case YY_CONNECTED_PEER_SERVER:
        break;
        case YY_CONNECTION_LOST:
          setMode(YY_MODE_CLIENT);
        break;
        case YY_DISCONNECTED:
        break;
      }
      currentStatus = yyStatus;
      printModeAndStatus();
    }

    setMode(updateTimeOuts());

    //Everytime for each mode:
    switch(currentMode) {
      case YY_MODE_NONE:
        digitalWrite(wifiLEDPin, LOW);
        break;
      case YY_MODE_CLIENT:
        digitalWrite(wifiLEDPin, LOW);
        break;
      case YY_MODE_PEER_CLIENT:
        digitalWrite(wifiLEDPin, HIGH);
        break;
      case YY_MODE_PEER_SERVER:
        digitalWrite(wifiLEDPin, HIGH);
        dnsServer.processNextRequest();
        break;
    }
  }

  return(currentStatus);
}

bool YoYoWiFiManager::setMode(yy_mode_t mode) {
  bool result = true;

  if(mode != currentMode) {
    switch(currentMode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        break;
      case YY_MODE_PEER_CLIENT: 
        break;
      case YY_MODE_PEER_SERVER:
        WiFi.softAPdisconnect(true);
        dnsServer.stop();
        break;
    }

    switch(mode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        updateClientTimeOut();
        //TODO: causes crash (on at least) ESP8266
        //if(startWebServerOnceConnected) startWebServer();
        //else stopWebServer(); //make sure it's stopped
        break;
      case YY_MODE_PEER_CLIENT: 
        updateClientTimeOut();
        startWebServer();
        break;
      case YY_MODE_PEER_SERVER:
        updateServerTimeOut();
        startPeerNetworkAsAP();
        startWebServer();
        break;
    }
    
    currentMode = mode;
    printModeAndStatus();
  }

  return(result);
}

void YoYoWiFiManager::printModeAndStatus() {
  switch(currentMode) {
    case YY_MODE_NONE:
      Serial.print("YY_MODE_NONE");
      break;
    case YY_MODE_CLIENT:
      Serial.print("YY_MODE_CLIENT");
      break;
    case YY_MODE_PEER_CLIENT: 
      Serial.print("YY_MODE_PEER_CLIENT");
      break;
    case YY_MODE_PEER_SERVER:
      Serial.print("YY_MODE_PEER_SERVER");
      break;
  }

  switch (currentStatus) {
    case YY_CONNECTED:
      Serial.println("\tYY_CONNECTED");
      break;
    case YY_IDLE_STATUS:
      Serial.println("\tYY_IDLE_STATUS");
      break;
    case YY_NO_SSID_AVAIL:
      Serial.println("\tYY_NO_SSID_AVAIL");
      break;
    case YY_SCAN_COMPLETED:
      Serial.println("\tYY_SCAN_COMPLETED");
      break;
    case YY_CONNECT_FAILED:
      Serial.println("\tYY_CONNECT_FAILED");
      break;
    case YY_CONNECTION_LOST:
      Serial.println("\tYY_CONNECTION_LOST");
      break;
    case YY_DISCONNECTED:
      Serial.println("\tYY_DISCONNECTED");
      break;
    case YY_CONNECTED_PEER_CLIENT:
      Serial.println("\tYY_CONNECTED_PEER_CLIENT");
      break;
    case YY_CONNECTED_PEER_SERVER:
      Serial.println("\tYY_CONNECTED_PEER_SERVER");
      break;
  }
}

bool YoYoWiFiManager::clientHasTimedOut() {
  return(clientTimeOutAtMs > 0 && millis() > clientTimeOutAtMs);
}

void YoYoWiFiManager::updateClientTimeOut() {
  clientTimeOutAtMs = millis() + WIFICLIENTTIMEOUT;
}

bool YoYoWiFiManager::serverHasTimedOut() {
  return(serverTimeOutAtMs > 0 && millis() > serverTimeOutAtMs);
}

void YoYoWiFiManager::updateServerTimeOut() {
  serverTimeOutAtMs = millis() + WIFISERVERTIMEOUT;
}

YoYoWiFiManager::yy_mode_t YoYoWiFiManager::updateTimeOuts() {
  yy_mode_t mode = currentMode;

  switch(currentStatus) {
    case YY_CONNECTED:
      updateClientTimeOut();
    break;
    case YY_CONNECTED_PEER_CLIENT:
      updateClientTimeOut();
    break;
    case YY_CONNECTED_PEER_SERVER:
      if(hasClients()) updateServerTimeOut();
    break;
    default:
      if(currentMode != YY_MODE_PEER_SERVER && clientHasTimedOut())
        mode = YY_MODE_PEER_SERVER;

      if(currentMode != YY_MODE_CLIENT && serverHasTimedOut() && settings && settings -> hasNetworkCredentials())
        mode = YY_MODE_CLIENT;
  }

  return(mode);
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
      onYoYoCommandGET(request);
    }
    else if (SPIFFS_ENABLED && SPIFFS.exists(request->url())) {
      sendFile(request, request->url());
    }
    else if (currentMode == YY_MODE_PEER_SERVER) {
      handleCaptivePortalRequest(request);
    }
    else if(request->url().equals("/")) {
      sendIndexFile(request);
    }
    else if(request->url().endsWith("/")) {
      sendFile(request, request->url() + "index.html");
    }
    else {
      request->send(404);
    }
  }
  else if (request->method() == HTTP_POST) {
    request->send(400); //POSTs are expected to have a body and then be processes by handleBody()
  }
  else {
    request->send(400);
  }
}

void YoYoWiFiManager::handleCaptivePortalRequest(AsyncWebServerRequest *request) {
    if (request->url().endsWith(".html") || 
              request->url().endsWith("/") ||
              request->url().endsWith("generate_204") ||
              request->url().endsWith("redirect"))  {
      sendIndexFile(request);
    }
    else if (request->url().endsWith("connecttest.txt") || 
              request->url().endsWith("ncsi.txt")) {
      request->send(200, "text/plain", "Microsoft NCSI");
    }
    else if (strstr(request->url().c_str(), "generate_204_") != NULL) {
      Serial.println("you must be huawei!");
      sendIndexFile(request);
    }
    else {
      request->send(304);
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
      //TODO: this limit seems artificial
      char *json = new char[1024];
      len = min(len, (unsigned int) 1024);
      for (int i = 0; i < len; i++)  json[i] = char(data[i]);

      Serial.printf("JSON > %s\n", json);

      StaticJsonDocument<1024> jsonDoc;
      if (!deserializeJson(jsonDoc, json)) {
        onYoYoCommandPOST(request, jsonDoc.as<JsonVariant>());
      }

      delete json;
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

  if (SPIFFS_ENABLED && SPIFFS.exists(path)) {
    request->send(SPIFFS, path, getContentType(path));
  }
  else {
    request->send(404);
  }
}

void YoYoWiFiManager::setRootIndexFile(String rootIndexFile) {
  this -> rootIndexFile = rootIndexFile;
}

void YoYoWiFiManager::sendIndexFile(AsyncWebServerRequest * request) {
  if (SPIFFS_ENABLED && SPIFFS.exists(rootIndexFile)) {
    sendFile(request, rootIndexFile);
  }
  else {
    request->send(200, "text/html", DEFAULT_INDEX_HTML);
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
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

void YoYoWiFiManager::onYoYoCommandGET(AsyncWebServerRequest *request) {
  bool success = false;

  if (request->url().equals("/yoyo/networks"))         getNetworks(request);
  else if (request->url().equals("/yoyo/clients"))     getClients(request);
  else if (request->url().equals("/yoyo/peers"))       getPeers(request);
  else if (request->url().equals("/yoyo/credentials")) getCredentials(request);
  else {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonDocument<1024> settingsJsonDoc;
    if(yoYoCommandGetHandler) {
      success = yoYoCommandGetHandler(request->url(), settingsJsonDoc.as<JsonVariant>());
    }

    if(success) {
      if(!settingsJsonDoc.isNull()) {
        String jsonString;
        serializeJson(settingsJsonDoc, jsonString);
        response->print(jsonString);
      }
      else response->print("{}");

      request->send(response);
    }
    else {
      request->send(400);
    }
  }
}

void YoYoWiFiManager::onYoYoCommandPOST(AsyncWebServerRequest *request, JsonVariant json) {
  if (request->url().equals("/yoyo/credentials")) {
    if(setCredentials(request, json)) {
      broadcastToPeersPOST(request->url(), json);
      delay(random(MAX_SYNC_DELAY));  //stop peers that are restarting together becoming synchronised
      connect();
    }
  }
  else {
    bool success = false;

    if(yoYoCommandPostHandler) {
      success = yoYoCommandPostHandler(request->url(), json);
    }
    request->send(success ? 200 : 404);
  }
}

void YoYoWiFiManager::broadcastToPeersPOST(String path, JsonVariant json) {
  if(currentMode == YY_MODE_PEER_SERVER) {
    int peerCount = countPeers();

    IPAddress *ipAddress = new IPAddress();
    for (int i = 0; i < peerCount; i++) {
      getPeerN(i, ipAddress, NULL);
      POST(ipAddress -> toString().c_str(), path.c_str(), json);
    }
    delete ipAddress;
  }
}

int YoYoWiFiManager::POST(const char *server, const char *path, JsonVariant json) {
  int httpResponseCode = -1;

  String jsonAsString(json.memoryUsage());
  if(serializeJson(json, jsonAsString) > 0) {
    httpResponseCode = POST(server, path, jsonAsString.c_str());
  }

  return(httpResponseCode);
}

int YoYoWiFiManager::POST(const char *server, const char *path, const char *payload) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("POST %s > %s\n", urlAsString.c_str(), payload);

  HTTPClient http;

  http.begin(urlAsString);
  httpResponseCode = http.POST(payload);

  // Read response
  Serial.print(http.getString());

  http.end();

  return(httpResponseCode);
}

int YoYoWiFiManager::GET(const char *server, const char *path, JsonDocument &json) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("GET %s\n", urlAsString.c_str());

  HTTPClient http;
  http.begin(urlAsString);

  httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    if(deserializeJson(json, http.getStream()) != DeserializationError::Ok) {
      httpResponseCode = -1;
    }
  }

  http.end();

  return(httpResponseCode);
}

int YoYoWiFiManager::GET(const char *server, const char *path, char *payload) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("GET %s\n", urlAsString.c_str());

  HTTPClient http;

  http.begin(urlAsString);
  httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    strcpy(payload, http.getString().c_str());
  }
  http.end();

  return(httpResponseCode);
}

void YoYoWiFiManager::getCredentials(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  response->print(getCredentialsAsJsonString());
  request->send(response);
}

String YoYoWiFiManager::getCredentialsAsJsonString() {
  String jsonString;

  StaticJsonDocument<1000> jsonDoc;
  getCredentialsAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getCredentialsAsJson(JsonDocument& jsonDoc) {
  //TODO: should in same structure - just missing the password field or replacing with *s
  JsonArray credentials = jsonDoc.createNestedArray();
 
  if(settings) {
    //Get all the credentials and turn them into json - but not passwords
    char *ssid = new char[32];

    int credentialsCount = settings -> getNumberOfNetworkCredentials();

    for (int i = 0; i < credentialsCount; i++) {
      settings -> getSSID(i, ssid);
      credentials.add(ssid);
    }
    delete ssid;
  }
}

bool YoYoWiFiManager::setCredentials(AsyncWebServerRequest *request, JsonVariant json) {
  bool success = setCredentials(json);
  request->send(success ? 200 : 400);

  return(success);
}

bool YoYoWiFiManager::setCredentials(JsonVariant json) {
  bool success = false;

  const char* ssid = json["ssid"];
  const char* password = json["password"];

  Serial.printf("setCredentials %s  %s\n", ssid, password);

  if(ssid && password) {
    addNetwork(ssid, password, true);
    success = true;
  }

  return(success);
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
  
  if(!jsonDoc.isNull()) {
    serializeJson(jsonDoc, jsonString);
  }
  else jsonString = "[]";

  return (jsonString);
}

void YoYoWiFiManager::getPeersAsJson(JsonDocument& jsonDoc) {
  IPAddress *ipAddress = new IPAddress();
  uint8_t *macAddress = new uint8_t[6];

  IPAddress *localIPAddress = NULL;
  
  if(currentMode == YY_MODE_PEER_SERVER) {
    localIPAddress = new IPAddress(WiFi.softAPIP());
    createNestedPeer(jsonDoc, localIPAddress, WiFi.softAPmacAddress(macAddress), true, true);

    int peerCount = countPeers();
    for (int i = 0; i < peerCount; i++) {
      getPeerN(i, ipAddress, macAddress);
      createNestedPeer(jsonDoc, ipAddress, macAddress);
    }
  }
  else if(currentMode == YY_MODE_PEER_CLIENT) {
    localIPAddress = new IPAddress(WiFi.localIP());
    GET(WiFi.gatewayIP().toString().c_str(), "/yoyo/peers", jsonDoc);
    
    //Correct the LOCALHOST attribution
    JsonArray peers = jsonDoc.as<JsonArray>();
    for (JsonVariant peer : peers) {
        if(peer["LOCALHOST"] == true) {
          peer.remove("LOCALHOST");
        }
        else if(peer["IP"] == localIPAddress->toString()) {
          peer["LOCALHOST"] = true;
        }
    }
  }
  else if(currentMode == YY_MODE_CLIENT) {
    //The only peer we know about is the local one:
    localIPAddress = new IPAddress(WiFi.localIP());
    createNestedPeer(jsonDoc, localIPAddress, WiFi.softAPmacAddress(macAddress), true);
  }

  if(localIPAddress) delete localIPAddress;

  delete ipAddress;
  delete macAddress;
}

bool YoYoWiFiManager::getPeerN(int n, IPAddress *ipAddress, uint8_t *macAddress) {
  bool success = false;

  switch(currentMode) {
    case YY_MODE_NONE:
      break;
    case YY_MODE_CLIENT:
      break;
    case YY_MODE_PEER_CLIENT:
      //TODO: implement
      break;
    case YY_MODE_PEER_SERVER:
        tcpip_adapter_sta_info_t station;
        int clientCount = countClients();
        int peerCount = 0;
        for(int i = 0; i < clientCount && !success; ++i) {
          station = adapter_sta_list.sta[i];
          if(isEspressif(station.mac)) {
            success = (peerCount == n);
            if(success) {
              if(ipAddress != NULL)   (*ipAddress) = (station.ip).addr;
              if(macAddress != NULL)  memcpy(macAddress, station.mac, sizeof(station.mac[0])*6);
            }
            peerCount++;
          }
        }
      break;
  }

  return(success);
}

void YoYoWiFiManager::createNestedPeer(JsonDocument& jsonDoc, IPAddress *ip, uint8_t *macAddress, bool localhost, bool gateway) {
    JsonObject peer = jsonDoc.createNestedObject();
    if(ip && macAddress) {
      peer["IP"] = ip -> toString();

      char *macAddressAsCStr = new char[18];
      mac_addr_to_c_str(macAddress, macAddressAsCStr);
      peer["MAC"] = macAddressAsCStr;
      delete macAddressAsCStr;

      if(localhost) peer["LOCALHOST"] = true;
      if(gateway)   peer["GATEWAY"] = true;
    }
}

bool YoYoWiFiManager::hasPeers() {
  return(countPeers() > 0);
}

int YoYoWiFiManager::countPeers() {
  int count = 0;

  switch(currentMode) {
    case YY_MODE_NONE:
      break;
    case YY_MODE_CLIENT:
      break;
    case YY_MODE_PEER_CLIENT:
      //TODO: this should use the list from the gateway and count those elements
      if(currentStatus == YY_CONNECTED_PEER_CLIENT) count = 1;  //is connected to the server
      break;
    case YY_MODE_PEER_SERVER:
      tcpip_adapter_sta_info_t station;
      int clientCount = countClients();
      for(int n = 0; n < clientCount; ++n) {
        station = adapter_sta_list.sta[n];
        if(isEspressif(station.mac)) count++;
      }
      break;
  }

  return(count);
}

void YoYoWiFiManager::getClients(AsyncWebServerRequest * request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  response->print(getClientsAsJsonString());
  request->send(response);
}

String YoYoWiFiManager::getClientsAsJsonString() {
  String jsonString;

  StaticJsonDocument<1000> jsonDoc;
  getClientsAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getClientsAsJson(JsonDocument& jsonDoc) {
  JsonArray clients = jsonDoc.createNestedArray();
 
  char *ipAddress = new char[17];
  char *macAddress = new char[18];

  if(currentMode == YY_MODE_PEER_SERVER) {
    tcpip_adapter_sta_info_t station;

    int clientCount = updateClientList();
    for(int n = 0; n < clientCount; ++n) {
      strcpy(ipAddress, ip4addr_ntoa(&(adapter_sta_list.sta[n].ip)));
      mac_addr_to_c_str(adapter_sta_list.sta[n].mac, macAddress);

      JsonObject client  = clients.createNestedObject();
      client["IP"] = ipAddress;
      client["MAC"] = macAddress; 
    }
  }
  else if(currentMode == YY_MODE_PEER_CLIENT) {
    //TODO - reques from server?
  }
  else if(currentMode == YY_MODE_CLIENT) {
    //TODO - empty?
  }

  delete ipAddress;
  delete macAddress;
}

int YoYoWiFiManager::updateClientList() {
  int count = 0;

  if(currentMode == YY_MODE_PEER_SERVER) {
    #if defined(ESP8266)
      struct station_info *stat_info;

      count = min(wifi_softap_get_station_num(), (uint8) ESP_WIFI_MAX_CONN_NUM);
      stat_info = wifi_softap_get_station_info();

      adapter_sta_list.num = count;

      int n=0;
      tcpip_adapter_sta_info_t station;
      while (count > 0 && stat_info != NULL) {
        memcpy(adapter_sta_list.sta[n].mac, stat_info->bssid, sizeof(stat_info->bssid[0])*6);
        adapter_sta_list.sta[n].ip = stat_info->ip;

        stat_info = STAILQ_NEXT(stat_info, next);
        n++;
      }
      wifi_softap_free_station_info();

    #elif defined(ESP32)
      esp_wifi_ap_get_sta_list(&wifi_sta_list);
      tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
      count = adapter_sta_list.num;
    #endif
  }
  else if(currentMode == YY_MODE_PEER_CLIENT) {
    //TODO
  }
  else if(currentMode == YY_MODE_CLIENT) {
    //TODO
  }

  return(count);
}

bool YoYoWiFiManager::hasClients() {
  return(countClients() > 0);
}

int YoYoWiFiManager::countClients() {
  int count = 0;

  if(currentMode == YY_MODE_PEER_SERVER) {
    count = updateClientList();
  }

  return(count);
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

  int n = scanNetworks();

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

int YoYoWiFiManager::scanNetworks() {
  int count = 0;

  if(lastScanNetworksAtMs == 0 || millis() > (lastScanNetworksAtMs + SCAN_NETWORKS_MIN_INT)) {
    lastScanNetworksAtMs = millis();

    #if defined(ESP8266)
      //ESP8266 scanNetworks() can only operate as async because of ESPAsyncWebServer > https://github.com/me-no-dev/ESPAsyncWebServer#scanning-for-available-wifi-networks
      //the consequence is that calls to function will fail to return any results if they initiate a scan
      count = WiFi.scanNetworks(true, false);

    #elif defined(ESP32)
      count = WiFi.scanNetworks(false, false);

    #endif
  }
  else {
    count = WiFi.scanComplete();
  }

  return(count);
}

bool YoYoWiFiManager::isEspressif(uint8_t *macAddress) {
  bool result = false;

  int oui = getOUI(macAddress[0], macAddress[1], macAddress[2]);
  int count = sizeof(ESPRESSIF_OUI) > 0 ? sizeof(ESPRESSIF_OUI)/sizeof(ESPRESSIF_OUI[0]) : 0;

  for(int n=0; n < count && !result; ++n) {
    result = (oui == ESPRESSIF_OUI[n]);
  }

  return(result);
}

bool YoYoWiFiManager::mac_addr_to_c_str(uint8_t *mac, char *str) {
  bool success = true;

  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return(success);
}

int YoYoWiFiManager::getOUI(char *mac) {
  int oui = 0;

  //basic format test ##:##:##:##:##:##
  if(strlen(mac) == 17) {
    int a, b, c, d, e, f;
    sscanf(mac, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
    
    oui = getOUI(a, b, c);
  }

  return(oui);
}

int YoYoWiFiManager::getOUI(uint8_t *mac) {
  int oui = getOUI(mac[0], mac[1], mac[2]);

  return(oui);
}

int YoYoWiFiManager::getOUI(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) {
  int oui = (a << 16) & 0xff0000 | (b << 8) & 0x00ff00 | c & 0x0000ff;

  return(oui);
}