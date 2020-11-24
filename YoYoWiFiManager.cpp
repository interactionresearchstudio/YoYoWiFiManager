#include "YoYoWiFiManager.h"

int YoYoWiFiManager::currentPairedStatus = remoteSetup;
int YoYoWiFiManager::currentSetupStatus = setup_pending;

YoYoWiFiManager::YoYoWiFiManager(uint8_t wifiLEDPin) {
  Serial.println("YoYoWiFiManager");
  this -> wifiLEDPin = wifiLEDPin;

  SPIFFS.begin();
}

boolean YoYoWiFiManager::autoConnect(char const *apName, char const *apPassword, bool force) {
  if(!credentials.available() || force) {
    Serial.println("no credentials available");
    if(joinPeerNetwork(apName, apPassword)) {
      //become client
      currentSetupStatus = setup_client;
    }
    else {
      //become server
      currentSetupStatus = setup_server;
      createPeerNetwork(apName, apPassword);
    }
  }
  else {
    Serial.println("credentials available");
    int N = credentials.getQuantity();
    for(int n = 0; n < N; ++n) {
      wifiMulti.addAP(credentials.getSSID(n) -> c_str(), credentials.getPassword(n) -> c_str());
      //TODO: handle timeout
    }
  }

  // while ((wifiMulti.run() != WL_CONNECTED)) {
  //   delay(500);
  //   yield();
  //   Serial.print(".");
  // }

  return(true);
}

bool YoYoWiFiManager::isConnected() {
  return !disconnected;
}

// Creates Access Point for other device to connect to
void YoYoWiFiManager::createPeerNetwork(char const *apName, char const *apPassword) {
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

// Scan and connect to peer wifi network. Returns true if joined.
boolean YoYoWiFiManager::joinPeerNetwork(char const *apName, char const *apPassword) {
  boolean joinedPeerNetwork = false;

  char foundNetwork[SSID_MAX_LENGTH * 2];
  if(findNetwork(apName, foundNetwork, true)) {
    Serial.printf("Found peer network: %s\n", foundNetwork);

    wifiMulti.addAP(foundNetwork, apPassword);
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

void YoYoWiFiManager::startWebServer() {
  Serial.println("startWebServer\n");

  webserver.addHandler(this).setFilter(ON_AP_FILTER);  //only when requested from AP
  webserver.begin();
}

void YoYoWiFiManager::connectToWifi(String credentials) {
  /*
  String _wifiCredentials = credentials;
  const size_t capacity = 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(2) + 150;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, _wifiCredentials);
  JsonArray ssid = doc["ssid"];
  JsonArray pass = doc["password"];
  if (ssid.size() > 0) {
    for (int i = 0; i < ssid.size(); i++) {
      if (isSSIDValid(ssid[i])) {
      wifiMulti.addAP(checkSsidForSpelling(ssid[i]).c_str(), pass[i]);
      }
    }
  } else {
    Serial.println("issue with wifi credentials, creating access point");
  }

  Serial.println("Connecting to Router");

  long wifiMillis = millis();
  bool connectSuccess = false;

  preferences.begin("scads", false);
  bool hasConnected = preferences.getBool("hasConnected");
  preferences.end();

  while (!connectSuccess) {

    uint8_t currentStatus = wifiMulti.run();

    printWifiStatus(currentStatus);

    if (currentStatus == WL_CONNECTED) {
      // Connected!

      if (!hasConnected) {
        // TODO add this to preferences manager
        preferences.begin("scads", false);
        preferences.putBool("hasConnected", true);
        preferences.end();
        hasConnected = true;
      }
      connectSuccess = true;
      break;
    }

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
      if (!hasConnected) {
        // Wipe credentials and reset
        Serial.println("Wifi connect failed, Please try your details again in the captive portal");
        // TODO add this to preferences manager
        preferences.begin("scads", false);
        preferences.putString("wifi", "");
        preferences.end();
        ESP.restart();
      }
    }

    delay(100);
    yield();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  disconnected = false;
  */
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

void YoYoWiFiManager::wifiCheck() {
  if (millis() - wificheckMillis > wifiCheckTime) {
    wificheckMillis = millis();
    if (wifiMulti.run() !=  WL_CONNECTED) {
      digitalWrite(wifiLEDPin, 1);
      disconnected = true;
    }
  }
}

bool YoYoWiFiManager::isSSIDValid(char const *ssid) {
  bool result = false;

  char foundNetwork[SSID_MAX_LENGTH * 2];
  if(findNetwork(ssid, foundNetwork, false, true, 2)) {
    if(strlen(foundNetwork) <= SSID_MAX_LENGTH) {
      result = true;
    }
  }

  return(result);
}

void YoYoWiFiManager::printWifiStatus(uint8_t status) {
  Serial.print("Status: ");
  switch (status) {
    case WL_CONNECTED:
      Serial.println("WL_CONNECTED");
      break;
    case WL_IDLE_STATUS:
      Serial.println("WL_IDLE_STATUS");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("WL_NO_SSID_AVAIL");
      break;
    case WL_SCAN_COMPLETED:
      Serial.println("WL_SCAN_COMPLETED");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("WL_CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("WL_CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      Serial.println("WL_DISCONNECTED");
      break;
  }
}

void YoYoWiFiManager::update() {
  dnsServer.processNextRequest();
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

  //if (!isResetting) {     //TODO: I don't think we need this anymore?
    if (request->method() == HTTP_GET) {
      if (request->url() == "/settings")    getSettings(request);
      else if (request->url() == "/scan")   getScan(request);
      else if (request->url() == "/peers")  getPeers(request);
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
  //}
}

void YoYoWiFiManager::handleBody(AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.print("handleBody: ");
  Serial.println(request->url());

  //if (!isResetting) {     //TODO: I don't think we need this anymore?
    if (request->method() == HTTP_POST) {
      if (request->url() == "/settings") {
        String json = "";
        for (int i = 0; i < len; i++)  json += char(data[i]);

        StaticJsonDocument<1024> credentialsJsonDoc;
        if (!deserializeJson(credentialsJsonDoc, json)) {
          if(setSettings(credentialsJsonDoc.as<JsonObject>())) request->send(200);

          else request->send(400);
        }
      }
      else if(request->url() == "/reboot") {
          String json = "";
          for (int i = 0; i < len; i++)  json += char(data[i]);

          StaticJsonDocument<256> rebootJsonDoc;
          if (!deserializeJson(rebootJsonDoc, json)) {
            //TODO:
            // int delayMs = rebootJsonDoc["delay"];

            // softReset(delayMs);
            // socket_server.textAll("RESTART");
          }

          request->send(200);
      }
      else {
        request->send(404);
      }
    }
  //}
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

void YoYoWiFiManager::getSettings(AsyncWebServerRequest *request) {
  Serial.println("getSettings");
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  StaticJsonDocument<1024> settingsJsonDoc;
  //TODO:
  /*
  settingsJsonDoc["local_mac"] = myID;
  settingsJsonDoc["local_ssid"] = "";
  settingsJsonDoc["local_pass_len"] = 0; //local_pass.length;
  settingsJsonDoc["remote_ssid"] = "";
  settingsJsonDoc["remote_pass_len"] = 0; //remote_pass.length;
  settingsJsonDoc["remote_mac"] = getRemoteMacAddress(1);
  settingsJsonDoc["local_paired_status"] = getCurrentPairedStatusAsString();
  Serial.println(getCurrentPairedStatusAsString());
  */

  String jsonString;
  serializeJson(settingsJsonDoc, jsonString);
  response->print(jsonString);

  request->send(response);
}

bool YoYoWiFiManager::setSettings(JsonVariant json) {
  bool success = false;

  Serial.println("getSettings");

  String local_ssid = json["local_ssid"].as<String>();
  String local_pass = json["local_pass"].as<String>();
  String remote_ssid = json["remote_ssid"].as<String>();
  String remote_pass = json["remote_pass"].as<String>();
  String remote_mac = json["remote_mac"].as<String>();

  if (remote_mac != "") {
    //TODO:
    //addToMacAddressJSON(remote_mac);
    success = true;
  }

  if (remote_pass != "" && remote_ssid != "" && local_ssid != "" && local_pass != "") {
    //TODO:
    /*
    addToWiFiJSON(local_ssid, local_pass);
    addToWiFiJSON(remote_ssid, remote_pass);
    sendWifiCredentials();
    */
    success = true;
  }
  else if (local_pass != "" && local_ssid != "" && remote_ssid == "" && remote_pass == "") {
    //TODO:
    /*
    addToWiFiJSON(local_ssid, local_pass);
    sendWifiCredentials();
    */
    success = true;
  }

  return (success);
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
  /*
  JsonArray peers = jsonDoc.createNestedArray();

  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);  

  for (int i = 0; i < stationList.num; ++i) {
    wifi_sta_info_t station = stationList.sta[i];

    JsonObject peer  = peers.createNestedObject();
    //network["mac"] = WiFi.SSID(i);
    //network["ip"] = WiFi.BSSIDstr(i);
  }
  */

  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
 
  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
 
  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
 
  for (int i = 0; i < adapter_sta_list.num; i++) {
 
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
 
    Serial.print("station nr ");
    Serial.println(i);
 
    Serial.print("MAC: ");
 
    for(int i = 0; i< 6; i++){
      
      Serial.printf("%02X", station.mac[i]);  
      if(i<5)Serial.print(":");
    }
 
    Serial.print("\nIP: ");  
    Serial.println(ip4addr_ntoa(&(station.ip)));    
  }
}

void YoYoWiFiManager::getScan(AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    response->print(getScanAsJsonString());
    request->send(response);
}

String YoYoWiFiManager::getScanAsJsonString() {
  String jsonString;

  StaticJsonDocument<1000> jsonDoc;
  getScanAsJson(jsonDoc);
  serializeJson(jsonDoc[0], jsonString);

  return (jsonString);
}

void YoYoWiFiManager::getScanAsJson(JsonDocument& jsonDoc) {
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