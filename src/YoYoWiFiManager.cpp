#include "YoYoWiFiManager.h"

YoYoWiFiManager::YoYoWiFiManager() {
}

void YoYoWiFiManager::init(YoYoNetworkSettingsInterface *settings, voidCallbackPtr onYY_CONNECTEDhandler, jsonCallbackPtr getHandler, jsonCallbackPtr postHandler, bool stopWebServerOnceConnected, int webServerPort, int wifiLEDPin, bool wifiLEDOn, yy_storage_t storageType, uint8_t csPin) {
  this -> settings = settings;
  this -> onYY_CONNECTEDhandler = onYY_CONNECTEDhandler;
  this -> yoYoCommandGetHandler = getHandler;
  this -> yoYoCommandPostHandler = postHandler;

  this -> stopWebServerOnceConnected = stopWebServerOnceConnected;
  this -> webServerPort = webServerPort;

  this -> wifiLEDPin = wifiLEDPin;
  this -> wifiLEDOn = wifiLEDOn;
  if(this -> wifiLEDPin >= 0) pinMode(wifiLEDPin, OUTPUT);

  WiFi.persistent(false); //YoYoWiFiManager manages the persistence of networks itself

  #if defined(ESP32)
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  #endif
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  this -> storageType = YY_NO_STORAGE;
  switch (storageType) {
    case YY_SPIFFS_STORAGE:
      if(SPIFFS.begin()) {
        fs = &SPIFFS;
        this -> storageType = YY_SPIFFS_STORAGE;
      }
      break;
    case YY_SD_STORAGE:
      //TODO: fix for ESP8266 SD object?
      if(SD.begin(csPin)) {
        fs = &SD;
        this -> storageType = YY_SD_STORAGE;
      }
      break;
  }
 
  peerNetworkSSID[0] = NULL;
  peerNetworkPassword[0] = NULL;

  randomSeed(getChipId());
}

boolean YoYoWiFiManager::begin(char const *apName, char const *apPassword, bool autoconnect, bool peerconnect) {
  running = true;

  addPeerNetwork((char *)apName, (char *)apPassword);
  wifiMulti.run();  //prioritise joining peer networks over known networks

  if(autoconnect && settings && settings -> hasNetworkCredentials()) {
    Serial.println("network credentials available");
    addKnownNetworks();
    setMode(YY_MODE_CLIENT, true);
  }
  else {
    if(peerconnect) setMode(YY_MODE_PEER_CLIENT, true); //attempt to join peer network;
    else            setMode(YY_MODE_PEER_SERVER, true);
  }

  return(true);
}

void YoYoWiFiManager::end() {
  Serial.println("YoYoWiFiManager::end()");

  setMode(YY_MODE_NONE, true);
  WiFi.disconnect();
  running = false;
}

uint32_t YoYoWiFiManager::getChipId() {
  uint32_t chipId = 0;

  #if defined(ESP8266)
    chipId = ESP.getChipId();

  #elif defined(ESP32)
    chipId = (uint32_t) ESP.getEfuseMac();

  #endif

  return(chipId);
}

void YoYoWiFiManager::connect(char const *ssid, char const *password) {
  addNetwork(ssid, password, false);
  connect();
}

void YoYoWiFiManager::connect() {
  running = true;
  //Once in YY_MODE_CLIENT mode - loop() will trigger wifiMulti.run()
  setMode(YY_MODE_CLIENT);
}

void YoYoWiFiManager::startPeerNetworkAsAP() {
  Serial.print("Wifi name:");
  Serial.println(peerNetworkSSID);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(peerNetworkSSID, peerNetworkPassword);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  //Resolve all hostnames to this IP address:
  dnsServer.start(DNS_PORT, "*", apIP);
}

void YoYoWiFiManager::stopPeerNetworkAsAP() {
  WiFi.softAPdisconnect(true);
  dnsServer.stop();
}

void YoYoWiFiManager::startWebServer() {
  if(webserver == NULL) {
    Serial.println("startWebServer");
    webserver = new AsyncWebServer(webServerPort);
    webserver -> addHandler(this);
    webserver -> begin();
  }
}

void YoYoWiFiManager::stopWebServer() {
  //TODO: this is crashing on the ESP8266:
  if(webserver != NULL) {
    // Serial.println("stopWebServer");
    // webserver -> end();
    // delete(webserver);
    // webserver = NULL;
  }
}

void YoYoWiFiManager::addPeerNetwork(char *ssid, char *password) {
  if(ssid) {
    strcpy(peerNetworkSSID, ssid);
    if(password != NULL) strcpy(peerNetworkPassword, password);

    addNetwork(peerNetworkSSID, peerNetworkPassword, false);
  }
}

void YoYoWiFiManager::addKnownNetworks() {
  if(settings) {
    int numberOfNetworkCredentials = settings->getNumberOfNetworkCredentials();
    char *ssid = new char[SSID_MAX_LENGTH];
    char *password = new char[PASSWORD_MAX_LENGTH];
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

  if(ssid && password) {
    if(strlen(ssid) > 0 && strlen(ssid) < SSID_MAX_LENGTH) {
      char *matchingSSID = new char[SSID_MAX_LENGTH];

      // if(findNetwork(ssid, matchingSSID, false, true, 2)) {
      //   ssid = matchingSSID;
      // }

      if(wifiMulti.addAP(ssid, password)) {
        if(save && settings) {
          success = settings -> addNetwork(ssid, password, true);
          if(!success) Serial.println("can't settings -> addNetwork()");
        }
        else success = true;
      }
      else {
        Serial.println("can't wifiMulti.addAP()");
      }
      delete matchingSSID;
    }
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

bool YoYoWiFiManager::isRunning() {
  return(running);
}

yy_status_t YoYoWiFiManager::getStatus() {
  yy_status_t yyStatus = YY_STOPPED;

  if(running) {
    uint8_t wlStatus = WiFi.status();
    if(currentMode != YY_MODE_PEER_SERVER && millis() > (lastUpdatedMultiAtMs + MIN_MULTIUPDATEINTERVAL)) {
      wlStatus = wifiMulti.run();
      lastUpdatedMultiAtMs = millis();
    }

    if(wlStatus == WL_CONNECTED) {
      if(WiFi.SSID().equals(peerNetworkSSID)) {
        yyStatus = YY_CONNECTED_PEER_CLIENT;
      }
      else yyStatus = YY_CONNECTED;
    }
    else if(currentMode == YY_MODE_PEER_SERVER && hasClients()) {
      yyStatus = YY_CONNECTED_PEER_SERVER;
    }
    else {
      yyStatus = (yy_status_t) wlStatus;  //Otherwise yy_status_t and wl_status_t are value compatible
    }
  }

  return(yyStatus);
}

uint8_t YoYoWiFiManager::loop() {
  if(running) {
    if(millis() > tickDueAtMs) tick();

    updateMode();
    yy_status_t yyStatus = getStatus();
    
    //Only when the status changes:
    if(currentStatus != yyStatus) {
      char *currentStatusString = new char[32];
      char *yyStatusString = new char[32];
      getStatusAsString(currentStatus, currentStatusString);
      getStatusAsString(yyStatus, yyStatusString);
      Serial.printf("STATUS:  %s\t>\t%s\n", currentStatusString, yyStatusString);
      delete currentStatusString, yyStatusString;

      switch(yyStatus) {
        case YY_CONNECTED:
          //implicitly in YY_MODE_CLIENT
          if(WiFi.SSID().equals(peerNetworkSSID)) {
            setMode(YY_MODE_PEER_CLIENT, true);
          }
          else {
            Serial.printf("Connected to: %s\n", WiFi.SSID().c_str());
            Serial.println(WiFi.localIP());

            if(settings) settings -> setLastNetwork(WiFi.SSID().c_str());
          }
          if(onYY_CONNECTEDhandler) {
            onYY_CONNECTEDhandler();
          }
        break;
        //implicitly in YY_MODE_PEER_CLIENT
        case YY_CONNECTED_PEER_CLIENT:
          Serial.printf("Connected to Peer Network: %s\n", WiFi.SSID().c_str());
          Serial.println(WiFi.localIP());
          setMode(YY_MODE_PEER_CLIENT, true);
        break;
        //implicitly in YY_MODE_PEER_SERVER
        case YY_CONNECTED_PEER_SERVER:
        break;
        case YY_CONNECTION_LOST:
        break;
        case YY_DISCONNECTED:
        break;
      }
      currentStatus = yyStatus;
    }

    //Everytime for each mode:
    switch(currentMode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        break;
      case YY_MODE_PEER_CLIENT:
        break;
      case YY_MODE_PEER_SERVER:
        dnsServer.processNextRequest();
        processBroadcastMessageList();
        break;
    }

    setMode(updateTimeOuts());
  }
  updateWifiLED();

  return(currentStatus);
}

void YoYoWiFiManager::tick() {
  if(promisedBytes > 0) {
    promisedBytes = max(0, promisedBytes - maxPromisedBytesPerTick);
  }
  tickDueAtMs = millis() + TICKINTERVAL_MS;
}

void YoYoWiFiManager::updateWifiLED() {
  if(this -> wifiLEDPin >= 0) {
    if(running) {
      bool blink = ((millis() / 1000) % 2) == 0;

      switch(currentMode) {
        case YY_MODE_NONE:
          setWifiLED(LOW);
          break;
        case YY_MODE_CLIENT:
          setWifiLED(LOW && blink);
          break;
        case YY_MODE_PEER_CLIENT:
          setWifiLED((currentStatus == YY_CONNECTED_PEER_CLIENT) ? HIGH : blink);
          break;
        case YY_MODE_PEER_SERVER:
          setWifiLED(HIGH);
          break;
      }
    }
    else setWifiLED(LOW);
  }
}

void YoYoWiFiManager::setWifiLED(bool value) {
  if(this -> wifiLEDPin >= 0) {
    value = !(wifiLEDOn ^ value);
    digitalWrite(wifiLEDPin, value);
  }
}

bool YoYoWiFiManager::peerNetworkSet() {
  return(peerNetworkSSID[0] != NULL);
}

void YoYoWiFiManager::setMode(yy_mode_t mode, bool update) {
  nextMode = mode;
  if(update) updateMode();
}

bool YoYoWiFiManager::updateMode() {
  bool result = false;

  if(promisedBytes > 0) {
    //waiting for jobs to complete
    return(false);
  }

  if(!broadcastMessageList.isNull() && broadcastMessageList.size() > 0) {
    //broadcast messages waiting to be sent
    return(false);
  }

  if(nextMode != currentMode) {
    delay(300);  //Allow any final transactions to complete before mode changes

    char *currentModeString = new char[32];
    char *nextModeString = new char[32];
    getModeAsString(currentMode, currentModeString);
    getModeAsString(nextMode, nextModeString);
    Serial.printf("MODE:\t%s\t>\t%s\n", currentModeString, nextModeString);
    delete currentModeString, nextModeString;

    switch(nextMode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        break;
      case YY_MODE_PEER_CLIENT:
        if(!peerNetworkSet()) return(false);
        break;
      case YY_MODE_PEER_SERVER:
        if(!peerNetworkSet()) return(false);
        break;
    }

    //From old mode:
    switch(currentMode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        break;
      case YY_MODE_PEER_CLIENT: 
        break;
      case YY_MODE_PEER_SERVER:
        stopPeerNetworkAsAP();
        break;
    }

    //To new mode:
    switch(nextMode) {
      case YY_MODE_NONE:
        break;
      case YY_MODE_CLIENT:
        WiFi.mode(WIFI_STA);
        Serial.printf("about to start server...\t%i\n", stopWebServerOnceConnected);
        if(stopWebServerOnceConnected) stopWebServer();
        else startWebServer();
        updateClientTimeOut();
        break;
      case YY_MODE_PEER_CLIENT: 
        WiFi.mode(WIFI_STA);
        startWebServer();
        updateClientTimeOut();
        break;
      case YY_MODE_PEER_SERVER:
        updateServerTimeOut();
        WiFi.mode(WIFI_AP_STA);
        delay(2000);
        startPeerNetworkAsAP();
        startWebServer();
        break;
    }
    currentMode = nextMode;

    //printWiFiDiag();
    result = true;
  }

  return(result);
}

void YoYoWiFiManager::printWiFiDiag() {
  Serial.print("localIP: ");
  Serial.println(WiFi.localIP());

  Serial.print("softAPIP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("subnetMask: ");
  Serial.println(WiFi.subnetMask());

  Serial.print("gatewayIP: ");
  Serial.println(WiFi.gatewayIP());

  WiFi.printDiag(Serial);

  Serial.println("-");
}

void YoYoWiFiManager::getModeAsString(yy_mode_t mode, char *string) {
  if(string != NULL) {
    switch(mode) {
      case YY_MODE_NONE:
        strcpy(string, "YY_MODE_NONE");
        break;
      case YY_MODE_CLIENT:
        strcpy(string, "YY_MODE_CLIENT");
        break;
      case YY_MODE_PEER_CLIENT: 
        strcpy(string, "YY_MODE_PEER_CLIENT");
        break;
      case YY_MODE_PEER_SERVER:
        strcpy(string, "YY_MODE_PEER_SERVER");
        break;
    }
  }
}

char * YoYoWiFiManager::getStatusAsString(char *string) {
  return(getStatusAsString(currentStatus, string));
}

char * YoYoWiFiManager::getStatusAsString(yy_status_t status, char *string) {
  if(string != NULL) {
    string[0] = '\0';

    switch(status) {
      case YY_STOPPED:
        strcpy(string, "YY_STOPPED");
        break;
      case YY_CONNECTED:
        strcpy(string, "YY_CONNECTED");
        break;
      case YY_IDLE_STATUS:
        strcpy(string, "YY_IDLE_STATUS");
        break;
      case YY_NO_SSID_AVAIL:
        strcpy(string, "YY_NO_SSID_AVAIL");
        break;
      case YY_SCAN_COMPLETED:
        strcpy(string, "YY_SCAN_COMPLETED");
        break;
      case YY_CONNECT_FAILED:
        strcpy(string, "YY_CONNECT_FAILED");
        break;
      case YY_CONNECTION_LOST:
        strcpy(string, "YY_CONNECTION_LOST");
        break;
      case YY_DISCONNECTED:
        strcpy(string, "YY_DISCONNECTED");
        break;
      case YY_CONNECTED_PEER_CLIENT:
        strcpy(string, "YY_CONNECTED_PEER_CLIENT");
        break;
      case YY_CONNECTED_PEER_SERVER:
        strcpy(string, "YY_CONNECTED_PEER_SERVER");
        break;
      default:
        break;
    }
  }
  return(string);
}

bool YoYoWiFiManager::clientHasTimedOut() {
  return(clientTimeOutAtMs > 0 && millis() > clientTimeOutAtMs);
}

void YoYoWiFiManager::updateClientTimeOut() {
  if(clientTimeOutAtMs == 0 || clientHasTimedOut()) {
    int timeoutMs = MIN_WIFICLIENTTIMEOUT + random(MIN_WIFICLIENTTIMEOUT);
    clientTimeOutAtMs = millis() + timeoutMs;
  }
}

bool YoYoWiFiManager::serverHasTimedOut() {
  return(serverTimeOutAtMs > 0 && millis() > serverTimeOutAtMs);
}

void YoYoWiFiManager::updateServerTimeOut() {
  if(serverTimeOutAtMs == 0 || serverHasTimedOut()) {
    int timeoutMs = MIN_WIFISERVERTIMEOUT + random(MIN_WIFISERVERTIMEOUT);
    serverTimeOutAtMs = millis() + timeoutMs;
  }
}

YoYoWiFiManager::yy_mode_t YoYoWiFiManager::updateTimeOuts() {
  yy_mode_t mode = nextMode;

  //if we aren't waiting for a mode change:
  if(currentMode == nextMode) {
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
        if(currentMode != YY_MODE_PEER_SERVER && peerNetworkSet() && clientHasTimedOut())
          mode = YY_MODE_PEER_SERVER;

        if(currentMode != YY_MODE_CLIENT && serverHasTimedOut() && settings && settings -> hasNetworkCredentials())
          mode = YY_MODE_CLIENT;
    }
  }

  return(mode);
}

//AsyncWebHandler
//===============

bool YoYoWiFiManager::canHandle(AsyncWebServerRequest *request) {
  //we can handle anything!

  return true;
}

void YoYoWiFiManager::handleRequest(AsyncWebServerRequest *request) {
  int result = 500;

  if (request->method() == HTTP_GET) {
    if(request->url().startsWith("/yoyo")) {
      result = onYoYoRequestGET(request);
    }
    else if (fileExists(request->url())) {
      result = sendFile(request, request->url());
    }
    else if (currentMode == YY_MODE_PEER_SERVER && getMimeType(request->url()).startsWith("text/")) {
      result = handleCaptivePortalRequest(request);
    }
    else if(request->url().equals("/")) {
      result = sendIndexFile(request);
    }
    else if(request->url().endsWith("/")) {
      result = sendFile(request, request->url() + "index.html");
    }
    else {
      request->send(404);
      result = 404;
    }
  }
  else if (request->method() == HTTP_POST) {
    request->send(400); //POSTs are expected to have a body and then be processes by handleBody()
    result = 400;
  }
  else if(request->method() == HTTP_HEAD) {
    request->send(400);
    result = 400;
  }
  else {
    request->send(400);
    result = 400;
  }

  Serial.printf("handleRequest: %s (%i)\n", request->url().c_str(), result);
}

int YoYoWiFiManager::handleCaptivePortalRequest(AsyncWebServerRequest *request) {
  int result = 500;

  //|| request->url().endsWith("/bag")

  if (request->url().endsWith(".html") || 
            request->url().endsWith("/") ||
            request->url().endsWith("generate_204") ||
            request->url().endsWith("redirect"))  {
    result = sendIndexFile(request);
  }
  else if (request->url().endsWith("connecttest.txt") || 
            request->url().endsWith("ncsi.txt")) {
    request->send(200, "text/plain", "Microsoft NCSI");
    result = 200;
  }
  else if (strstr(request->url().c_str(), "generate_204_") != NULL) {
    Serial.println("you must be huawei!");
    result = sendIndexFile(request);
  }
  else {
    request->send(304); //304 Not Modified client redirection response code indicates that there is no need to retransmit the requested resources
    result = 304;
  }

  return(result);
}

void YoYoWiFiManager::handleBody(AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
  int result = 0;

  if (request->method() == HTTP_GET) {
    request->send(400); //GETs are expected to have no body and then be processes by handleRequest()
    result = 400;
  }
  else if (request->method() == HTTP_POST) {
    if(request->url().startsWith("/yoyo")) {
      result = onYoYoRequestPOST(data, len, request);
    }
    else {
      request->send(404);
      result = 404;
    }
  }
  else if (request->method() == HTTP_DELETE) {
    if(request->url().startsWith("/yoyo")) {
      result = onYoYoRequestDELETE(data, len, request);
    }
    else {
      request->send(404);
      result = 404;
    }
  }
  else {
    request->send(400);
    result = 400;
  }

  Serial.printf("handleRequest: %s (%i)\n", request->url().c_str(), result);
}

int YoYoWiFiManager::fileExists(String path, String defaultIndexFile) {
  int result = 0;

  switch(fileSize(path, defaultIndexFile)) {
    case -1:
      result = 0; //file does not exist
      break;
    case 0:
      result = 2; //Attempt to catch: E (18543) vfs_fat: open: no free file descriptors
      break;
    default:
      result = 1; //file does exist
      break;
  }

  return(result);
}

int YoYoWiFiManager::fileSize(String path, String defaultIndexFile) {
  int result = -1;

  if(fs) {
    if(path.endsWith("/")) path += defaultIndexFile;

    File f = fs->open(path,"r");
    if(f) {
      result = f.size();
      f.close();
    }
  }

  return(result);
}

int YoYoWiFiManager::sendFile(AsyncWebServerRequest * request, String path, String defaultIndexFile) {
  int result = 500;
  
  if (fs) {
    if(promisedBytes < maxPromisedBytesPerTick) {
      if(path.endsWith("/")) path += defaultIndexFile;

      switch(fileExists(path)) {
        case 0:
          result = 404;
          request->send(404);
          break;
        case 1:
          result = 200;
          promisedBytes += fileSize(path);
          request->send(*fs, path, getMimeType(path));
          break;
        case 2: //Attempt to catch: E (18543) vfs_fat: open: no free file descriptors
          result = sendTooManyRequests(request);
          break;
      }
    }
    else {
      result = sendTooManyRequests(request);
    }
  }
  else {
    request->send(404);
    result = 404;
  }

  return(result);
}

int YoYoWiFiManager::sendTooManyRequests(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse(429); //Too Many Requests

  int delaySec = (int)random(1,6);  //for requests that occur together attempt to space them out
  if(promisedBytes>0) {
    delaySec += ((promisedBytes/maxPromisedBytesPerTick) * TICKINTERVAL_MS)/1000;
  }
  response->addHeader("Retry-After", String(delaySec));

  return(429);
}

void YoYoWiFiManager::setRootIndexFile(String rootIndexFile) {
  this -> rootIndexFile = rootIndexFile;
}

int YoYoWiFiManager::sendIndexFile(AsyncWebServerRequest * request) {
  int result = 500;

  if (fileExists(rootIndexFile)) {
    result = sendFile(request, rootIndexFile);
  }
  else {
    request->send(200, "text/html", DEFAULT_INDEX_HTML);
    result = 200;
  }

  return(result);
}

String YoYoWiFiManager::getMimeType(String filename) {
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

int YoYoWiFiManager::onYoYoRequestGET(AsyncWebServerRequest *request) {
  int result = 200;
  
  if (request->url().equals("/yoyo/networks"))         getNetworks(request);
  else if (request->url().equals("/yoyo/clients"))     getClients(request);
  else if (request->url().equals("/yoyo/peers"))       getPeers(request);
  else if (request->url().equals("/yoyo/credentials")) getCredentials(request);
  else {
    result = onYoYoMessageGET(request);
  }

  return(result);
}

int YoYoWiFiManager::onYoYoMessageGET(AsyncWebServerRequest *request) {
  bool success = false;

  DynamicJsonDocument message(1024);
  if(yoYoCommandGetHandler) {
    message["path"] = (char *) request->url().c_str();
    success = yoYoCommandGetHandler(message.as<JsonVariant>());
    serializeJson(message, Serial);
    Serial.println();
  }

  if(success) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    if(!message["payload"].isNull()) {
      response->print(message["payload"].as<String>());
    }
    else response->print("{}");

    request->send(response);
  }
  else {
    request->send(400);
  }

  return(success?200:400);
} 

int YoYoWiFiManager::onYoYoRequestPOST(uint8_t *data, size_t len, AsyncWebServerRequest *request) {
  int result = 500;
  
  if(request -> contentType().equals("application/json")){
    //TODO: this limit seems artificial
    char *json = new char[1024];
    len = min(len, (unsigned int) 1024-1);
    int i = 0;
    for (; i < len; i++)  json[i] = char(data[i]);
    json[i+1] = '\0';

    DynamicJsonDocument message(1024);
    DynamicJsonDocument payload(1024);
    //NB cast to (const char*) forces a copy by value as will shortly delete json object:
    if(deserializeJson(payload, (const char*)json) == DeserializationError::Ok) {
      message["payload"] = payload;
      message["path"] = (char *) request->url().c_str();
      message["method"] = "POST";

      result = onYoYoMessagePOST(message.as<JsonVariant>(), request); //does own request response
    }
    else {
      Serial.printf("can't deserialize json\n%s\n\n", (const char*)json);
      request->send(400);
      result = 400;
    }

    delete json;
  }
  else if(request -> contentType().equals("multipart/form-data")) {
    //FILE UPLOAD
    request->send(200);
    result = 200;
  }
  else {
    Serial.printf("unknown content type: %s\n", request -> contentType().c_str());
    request->send(400);
    result = 400;
  }

  return(result);
}

int YoYoWiFiManager::onYoYoRequestUPLOAD(uint8_t *data, size_t len, AsyncWebServerRequest *request) {
  int result = 500;

  if (request->url().equals("/yoyo/upload")) {
    request->send(200);
    result = 200;
  }
  else {
    request->send(404);
    result = 404;
  }

  return(result);
}

int YoYoWiFiManager::onYoYoMessagePOST(JsonVariant message, AsyncWebServerRequest *request) {
  int result = 500;
  
  bool success = false;
  Serial.println("onYoYoMessagePOST: " + message["path"].as<String>());

  if (message["path"] == "/yoyo/broadcast") {
    //TODO: request to broadcast a message from a peer - 404 unless YY_MODE_PEER_SERVER
    request->send(404);
    result = 404;
  }
  else if (message["path"] == "/yoyo/credentials") {
    if(setCredentials(message["payload"])) {
      request->send(200, "application/javascript", getCredentialsAsJsonString()); //TODO: is this the problem?
      result = 200;

      connect();  //this requests YY_MODE_CLIENT mode - which will be accessed on next loop() call
      message["broadcast"] = true;
      success = true;
    }
    else {
      Serial.println("can't set credentials from json");
      request->send(400);
      result = 400;
    }
  }
  else {
    //Bespoke endpoint
    if(yoYoCommandPostHandler) {
      success = yoYoCommandPostHandler(message);
    }

    if(success) {
      if(message["payload"].size() > 0) {
        String payload;
        serializeJson(message["payload"], payload);
        request->send(200, "application/javascript", payload.c_str());
      }
      else request->send(200);
      result = 200;
    }
    else {
      request->send(400);
      result = 400;
    }
  }
  //when the response is sent, the client is closed and freed from the memory

  if(success) {
    if(currentMode == YY_MODE_PEER_SERVER && message["broadcast"] == true) {
      addBroadcastMessage(message);
    }
  }

  return(result);
}

void YoYoWiFiManager::addBroadcastMessage(JsonVariant message) {
  if(currentMode == YY_MODE_PEER_SERVER) {
    broadcastMessageList.add(message);
  }
}

void YoYoWiFiManager::processBroadcastMessageList() {
  if(!broadcastMessageList.isNull() && broadcastMessageList.size() > 0) {
    broadcastMessage(broadcastMessageList[0]);
    broadcastMessageList.remove(0);
  }
}

bool YoYoWiFiManager::broadcastMessage(JsonVariant message) {
  bool result = false;

  if(currentMode == YY_MODE_PEER_SERVER) {
    int peerCount = countPeers();

    if(peerCount > 0) {
      result = true;
      IPAddress *ipAddress = new IPAddress();
      for (int i = 0; i < peerCount; i++) {
        getPeerN(i, ipAddress, NULL);
        if(message["method"] == "POST") POST(ipAddress -> toString().c_str(), message["path"], message["payload"]);
        //TODO: consider the other method types
      }
      delete ipAddress;
    }
  }

  return(result);
}

int YoYoWiFiManager::onYoYoRequestDELETE(uint8_t *data, size_t len, AsyncWebServerRequest *request) {
  //TODO fix this!

  //TODO: this limit seems artificial
  // char *json = new char[1024];
  // len = min(len, (unsigned int) 1024-1);
  // int i = 0;
  // for (; i < len; i++)  json[i] = char(data[i]);
  // json[i+1] = '\0';

  // DynamicJsonDocument message(1024);
  // message["path"] = (char *) request->url().c_str();
  // message["method"] = "DELETE";
  // if (!deserializeJson(jsonDoc, json)) {
  //   onYoYoMessageDELETE(message.as<JsonVariant>(), request);
  // }

  // delete json;

  return(500);
}

int YoYoWiFiManager::onYoYoMessageDELETE(JsonVariant message, AsyncWebServerRequest *request) {
  int result = 500;
  
  bool success = false;
  Serial.println("onYoYoMessageDELETE: " + message["path"].as<String>());

  if (message["path"] == "/yoyo/credentials") {
    //TODO: implement delete using YoYoSettings::removeNetwork()
    success = true;
  }
  result = success ? 200 : 404;
  request->send(result);

  return(result);
}

int YoYoWiFiManager::POST(const char *server, const char *path, JsonVariant payload, char *response) {
  int httpResponseCode = -1;

  String jsonAsString;
  jsonAsString.reserve(payload.memoryUsage());
  if(serializeJson(payload, jsonAsString) > 0) {
    httpResponseCode = POST(server, path, jsonAsString.c_str(), "application/json", response);
    if(response) {
      //TODO: parse json
    }
  }

  return(httpResponseCode);
}

int YoYoWiFiManager::POST(const char *server, const char *path, const char *payload, char *contentType, char *response) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("POST %s > %s\n", urlAsString.c_str(), payload);

  HTTPClient http;
  WiFiClient client;
  http.begin(client, urlAsString);
  if(contentType) http.addHeader("Content-Type", contentType);

  httpResponseCode = http.POST(payload);

  if(response && httpResponseCode > 0) {
    //#TODO: test length?
    strcpy(response, http.getString().c_str());
  }

  client.stop();
  http.end();

  return(httpResponseCode);
}

int YoYoWiFiManager::GET(const char *server, const char *path, JsonDocument &response) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("GET %s\n", urlAsString.c_str());

  HTTPClient http;
  WiFiClient client;
  http.begin(client, urlAsString);

  httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    if(deserializeJson(response, http.getStream()) != DeserializationError::Ok) {
      httpResponseCode = -1;
    }
  }
  client.stop();
  http.end();

  return(httpResponseCode);
}

int YoYoWiFiManager::GET(const char *server, const char *path, char *response) {
  int httpResponseCode = -1;

  String urlAsString = "http://" + String(server) + String(path);
  Serial.printf("GET %s\n", urlAsString.c_str());

  HTTPClient http;
  WiFiClient client;
  http.begin(client, urlAsString);

  httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    //#TODO: test length?
    strcpy(response, http.getString().c_str());
  }
  client.stop();
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

  DynamicJsonDocument jsonDoc(1024);
  getCredentialsAsJson(jsonDoc);

  if(!jsonDoc.isNull()) {
    serializeJson(jsonDoc, jsonString); //TODO: test length
  }
  else jsonString = "[]";

  return (jsonString);
}

void YoYoWiFiManager::getCredentialsAsJson(JsonDocument& jsonDoc) {
  //TODO: should in same structure - just missing the password field or replacing with *s
 
  if(settings) {
    //Get all the credentials and turn them into json - but not passwords
    char *ssid = new char[SSID_MAX_LENGTH];
    char *password = new char[PASSWORD_MAX_LENGTH];

    int credentialsCount = settings -> getNumberOfNetworkCredentials();
    int lastNetwork = settings -> getLastNetwork();

    for (int n = 0; n < credentialsCount; n++) {
      settings -> getSSID(n, ssid);
      settings -> getPassword(n, password);

      //star out password - while maintaining length
      for(int i=0; i < strlen(password); ++i) password[i] = '*';

      JsonObject network  = jsonDoc.createNestedObject();
      network["ssid"] = ssid;
      network["password"] = password;
      if(n == lastNetwork) network["lastnetwork"] = true;
    }
    delete ssid, password;
  }
}

bool YoYoWiFiManager::setCredentials(JsonVariant json) {
  bool success = false;

  if(settings) {
    char *ssid = new char[SSID_MAX_LENGTH];
    char *password = new char[PASSWORD_MAX_LENGTH];

    strcpy(ssid, json["ssid"]);
    strcpy(password, json["password"]);

    if(ssid && password) {
      Serial.printf("setCredentials %s  %s\n", ssid, password);
      success = addNetwork(ssid, password, true);
    }
    else {
      Serial.println("can't extract ssid and network from json");
    }

    delete password;
    delete ssid;
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

  DynamicJsonDocument jsonDoc(1024);
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

  DynamicJsonDocument jsonDoc(1024);
  getClientsAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getClientsAsJson(JsonDocument& jsonDoc) {
  JsonArray clients = jsonDoc.createNestedArray();

  if(currentMode == YY_MODE_PEER_SERVER) {
    char *ipAddress = new char[17];
    char *macAddress = new char[18];
    tcpip_adapter_sta_info_t station;

    int clientCount = updateClientList();
    for(int n = 0; n < clientCount; ++n) {
      strcpy(ipAddress, ip4addr_ntoa(&(adapter_sta_list.sta[n].ip)));
      mac_addr_to_c_str(adapter_sta_list.sta[n].mac, macAddress);

      JsonObject client  = clients.createNestedObject();
      client["IP"] = ipAddress;
      client["MAC"] = macAddress; 
    }

    delete ipAddress;
    delete macAddress;
  }
  else if(currentMode == YY_MODE_PEER_CLIENT) {
    //Empty
  }
  else if(currentMode == YY_MODE_CLIENT) {
    //Empty
  }
}

int YoYoWiFiManager::updateClientList() {
  int count = 0;

  if(millis() > lastUpdatedClientListAtMs + MIN_CLIENTLISTUPDATEINTERVAL) {
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
      //NOTHING TO DO
    }
    else if(currentMode == YY_MODE_CLIENT) {
      //NOTHING TO DO
    }
    currentClientCount = count;
    lastUpdatedClientListAtMs = millis();
  }
  else {
    count = currentClientCount;
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

  DynamicJsonDocument jsonDoc(1024);
  getNetworksAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getNetworksAsJson(JsonDocument& jsonDoc) {
  JsonArray networks = jsonDoc.createNestedArray();

  int n = scanNetworks();

  for (int i = 0; i < n; ++i) {
    String networkSSID = WiFi.SSID(i);
    if (networkSSID.length() < SSID_MAX_LENGTH) {
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