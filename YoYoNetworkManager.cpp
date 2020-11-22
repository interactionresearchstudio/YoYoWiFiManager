#include "YoYoNetworkManager.h"

YoYoWsClient YoYoNetworkManager::wsClient;
int YoYoNetworkManager::currentPairedStatus = remoteSetup;
int YoYoNetworkManager::currentSetupStatus = setup_pending;

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
      wsClient.setWsEventHandler(webSocketClientEvent);
      wsClient.begin();
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

void YoYoNetworkManager::setupCaptivePortal() {
  dnsServer.start(DNS_PORT, "*", apIP);
}

void YoYoNetworkManager::setupLocalServer() {
  //TODO: empty
}

void YoYoNetworkManager::setupSocketIOEvents() {
  //TODO: empty
}

void YoYoNetworkManager::webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!\n");
      //Hot fix for when client doesn't catch RESTART command
      // TODO softReset(4000);
      break;
    case WStype_CONNECTED:
      Serial.println("Connected!");
      //wsClient.sendMessage(getJSONMac().c_str());
      wsClient.sendMessage("hi");
      break;
    case WStype_TEXT:
      Serial.println("Text received");
      #ifdef DEV
      Serial.println((char *)payload);
      #endif
      String output = (char *)payload;
      if (output == "RESTART") {
        // TODO softReset(4000);
        Serial.println("i'm going to reset");
      } else {
        decodeWsData((char *)payload);
      }
      break;
  }
}

// determines if data received from websocket is mac address or wifi credentials
void YoYoNetworkManager::decodeWsData(const char* data) {
  const size_t capacity = 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(2) + 150;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, (const char*)data);
  if (doc.containsKey("mac")) {
    JsonArray mac = doc["mac"];
    String MAC = mac[0];
    Serial.println("I received a MAC address");
    Serial.println(MAC);
    if (MAC != "") {
      //save to preferences
      // TODO addToMacAddressJSON(MAC);
      if (currentSetupStatus != setup_client) {
        // TODO wsClient.sendMessage(getJSONMac());
      }
    } else {
      Serial.println("remote MAC address incorrect");
    }
  } else if (doc.containsKey("ssid")) {
    String remoteSSID = doc["ssid"][0];
    Serial.println("I received a SSID");
    if (remoteSSID != NULL) {
      if (doc.containsKey("password")) {
        String remotePASS = doc["password"][0];
        Serial.println("I received a Password");
        JsonArray wifi = doc["ssid"];
        for (int i = 0; i < wifi.size(); i++) {
          // TODO addToWiFiJSON(doc["ssid"][i], doc["password"][i]);
        }
      }
    } else {
      Serial.println("remote ssid empty");
    }
  } else {
    Serial.println("Incorrect data format");
    Serial.println(data);
  }
}
