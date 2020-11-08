#include "YoYoNetworkManager.h"

void YoYoNetworkManager::loadCredentials() {
  preferences.begin("scads", false);
  wifiCredentials = preferences.getString("wifi", "");
  macCredentials = preferences.getString("mac", "");
  preferences.end();
}

void YoYoNetworkManager::begin(uint8_t wifiLEDPin) {
  this -> wifiLEDPin = wifiLEDPin;
  yoyoWifi.attachLed(wifiLEDPin);

  loadCredentials();
  setPairedStatus();
  myID = yoyoWifi.generateID();

  if (wifiCredentials == "" || getNumberOfMacAddresses() < 2) {
    Serial.println("Scanning for available SCADS");
    boolean foundLocalSCADS = yoyoWifi.scanAndConnectToLocalSCADS();
    if (!foundLocalSCADS) {
      //become server
      currentSetupStatus = setup_server;
      yoyoWifi.createSCADSAP();
      setupCaptivePortal();
      setupLocalServer();
    }
    else {
      //become client
      currentSetupStatus = setup_client;
      setupSocketClientEvents();
    }
  }
  else {
    Serial.print("List of Mac addresses:");
    Serial.println(macCredentials);
    //connect to router to talk to server
    digitalWrite(wifiLEDPin, 0);
    yoyoWifi.connectToWifi(wifiCredentials);
    //checkForUpdate();
    setupSocketIOEvents();
    currentSetupStatus = setup_finished;
    Serial.println("setup complete");
  }
}

void YoYoNetworkManager::update() {
  checkReset();
}

void YoYoNetworkManager::factoryReset() {
  Serial.println("factoryReset");

  preferences.begin("scads", false);
  preferences.clear();
  preferences.end();
  currentSetupStatus = setup_pending;

  ESP.restart();
}

void YoYoNetworkManager::softReset(int delayMs) {
  if (isResetting == false) {
    isResetting = true;
    resetTime = millis() + delayMs;
  }
}

void YoYoNetworkManager::checkReset() {
  if (isResetting) {
    if (millis() > resetTime + resetDurationMs) {
      ESP.restart();
    }
  }
}

void YoYoNetworkManager::setPairedStatus() {
  int numberOfMacAddresses = getNumberOfMacAddresses();
  if (numberOfMacAddresses == 0) {
    Serial.println("setting up JSON database for mac addresses");
    preferences.clear();
    addToMacAddressJSON(myID);
  }
  else if (numberOfMacAddresses < 2) {
    //check it has a paired mac address
    Serial.println("Already have local mac address in preferences, but nothing else");
  }
  else {
    currentPairedStatus = pairedSetup;
    Serial.println("Already has one or more paired mac address");
  }
}

int YoYoNetworkManager::getNumberOfMacAddresses() {
  int numberOfMacAddresses = 0;

  //Returns the number of mac address in JSON array
  preferences.begin("scads", false);
  String macCredentials = preferences.getString("mac", "");
  preferences.end();

  if (macCredentials != "") {
    const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
    DynamicJsonDocument addresses(capacity);
    deserializeJson(addresses, macCredentials);
    numberOfMacAddresses = addresses["mac"].size();
  }

  return (numberOfMacAddresses);
}

void YoYoNetworkManager::addToMacAddressJSON(String addr) {
  // appends mac address to memory json array if isn't already in it, creates the json array if it doesnt exist
  preferences.begin("scads", false);
  String macAddressList = preferences.getString("mac", "");
  if (macAddressList != "") {
    const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
    DynamicJsonDocument addresses(capacity);
    deserializeJson(addresses, macAddressList);
    JsonArray mac = addresses["mac"];
    inList = false;
    for ( int i = 0; i < mac.size(); i++) {
      if (mac[i] == addr) {
        inList = true;
        Serial.println("mac address already in list");
        break;
      }
    }
    if (inList == false) {
      mac.add(addr);
      Serial.print("adding ");
      Serial.print(addr);
      Serial.println(" to the address list");
      macAddressList = "";
      serializeJson(addresses, macAddressList);
      Serial.println(macAddressList);
      preferences.putString("mac", macAddressList);
    }
  } else {
    const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
    DynamicJsonDocument addresses(capacity);
    JsonArray macArray = addresses.createNestedArray("mac");
    macArray.add(addr);
    macAddressList = "";
    serializeJson(addresses, macAddressList);
    preferences.putString("mac", macAddressList);
    Serial.print("creating json object and adding the local mac ");
    Serial.print(addr);
    Serial.println(" to the address list");
  }
  preferences.end();
}

/*
boolean YoYoNetworkManager::scanAndConnectToLocalSCADS() {
  boolean foundLocalSCADS = false;

  // WiFi.scanNetworks will return the number of networks found
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        scads_ssid = WiFi.SSID(i);
        if (scads_ssid.indexOf("Yo-Yo-") > -1) {
          Serial.println("Found YOYO");
          foundLocalSCADS = true;
          wifiMulti.addAP(scads_ssid.c_str(), scads_pass.c_str());
          while ((wifiMulti.run() != WL_CONNECTED)) {
            delay(500);
            Serial.print(".");
          }
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
  }
  return (foundLocalSCADS);
}
*/
/*
void YoYoNetworkManager::createSCADSAP() {
  //Creates Access Point for other device to connect to
  scads_ssid = "Yo-Yo-" + generateID();
  Serial.print("Wifi name:");
  Serial.println(scads_ssid);

  WiFi.mode(WIFI_AP);
  delay(2000);

  WiFi.persistent(false);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(scads_ssid.c_str(), scads_pass.c_str());
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
}
*/

/*
void YoYoNetworkManager::connectToWifi(String credentials) {
  String _wifiCredentials = credentials;
  const size_t capacity = 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(2) + 150;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, _wifiCredentials);
  JsonArray ssid = doc["ssid"];
  JsonArray pass = doc["password"];
  if (ssid.size() > 0) {
    for (int i = 0; i < ssid.size(); i++) {
      if (isWifiValid(ssid[i])) {
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

    //#ifdef DEV
      printWifiStatus(currentStatus);
    //#endif

    if (currentStatus == WL_CONNECTED) {
      // Connected!

      if (!hasConnected) {
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
}

void YoYoNetworkManager::printWifiStatus(uint8_t status) {
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
*/
void YoYoNetworkManager::setupCaptivePortal() {
  dnsServer.start(DNS_PORT, "*", apIP);
}

void YoYoNetworkManager::setupLocalServer() {
  //TODO: empty
}

void YoYoNetworkManager::setupSocketClientEvents() {
  //TODO: empty
}

void YoYoNetworkManager::setupSocketIOEvents() {
  //TODO: empty
}

/*
bool YoYoNetworkManager::isWifiValid(String incomingSSID) {
  int n = WiFi.scanNetworks();
  int currMatch = 255;
  int prevMatch = currMatch;
  int matchID;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    Serial.println("can't find any wifi in the area");
    return false;
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        currMatch = Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2;
        if (Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2) {
          if (currMatch < prevMatch) {
            prevMatch = currMatch;
            matchID = i;
          }
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
    if (prevMatch != 255) {
      Serial.println("Found a match!");
      return true;
    } else {
      Serial.println("can't find any wifi that are close enough matches in the area");
      return false;
    }
  }
}
*/

/*
String YoYoNetworkManager::checkSsidForSpelling(String incomingSSID) {
  int n = WiFi.scanNetworks();
  int currMatch = 255;
  int prevMatch = currMatch;
  int matchID;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    Serial.println("can't find any wifi in the area");
    return incomingSSID;
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        currMatch = Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2;
        if (Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2) {
          if (currMatch < prevMatch) {
            prevMatch = currMatch;
            matchID = i;
          }
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
    if (prevMatch != 255) {
      Serial.println("Found a match!");
      return WiFi.SSID(matchID);
    } else {
      Serial.println("can't find any wifi that are close enough matches in the area");
      return incomingSSID;
    }
  }
}
*/
