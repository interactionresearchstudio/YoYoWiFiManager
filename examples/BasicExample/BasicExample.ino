#include "YoYoWiFiManager.h"
#include "Settings.h"

YoYoWiFiManager wifiManager;
Settings *settings;

uint8_t currentStatus = YoYoWiFiManager::YY_IDLE_STATUS;

void setup() {
  Serial.begin(115200);

  settings = new Settings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  
  wifiManager.init(settings, onYoYoCommandGET, onYoYoCommandPOST);
  wifiManager.begin("YoYoMachines", "blinkblink", true);
}

/*
      YY_NO_SHIELD        = WL_NO_SHIELD,
      YY_IDLE_STATUS      = WL_IDLE_STATUS,
      YY_NO_SSID_AVAIL    = WL_NO_SSID_AVAIL,
      YY_SCAN_COMPLETED   = WL_SCAN_COMPLETED,
      YY_CONNECTED        = WL_CONNECTED,
      YY_CONNECT_FAILED   = WL_CONNECT_FAILED,
      YY_CONNECTION_LOST  = WL_CONNECTION_LOST,
      YY_DISCONNECTED     = WL_DISCONNECTED,
      YY_CONNECTED_PEER_CLIENT,
      YY_CONNECTED_PEER_SERVER
*/

void loop() {
  uint8_t s = wifiManager.update();
  if(s != currentStatus) {
    currentStatus = s;
    printStatus();
  }

  if(currentStatus == WL_CONNECTED) {

  }
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  //put the results of this into the json object
  Serial.println("onYoYoCommandGET " + url);
  
  if(url.equals("/yoyo/settings") && settings) {
    success = true;
    json.set(*settings);
  }

 return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  //read this json object and do something
  Serial.println("onYoYoCommandPOST " + url);
  serializeJson(json, Serial);

  if(url.equals("/yoyo/settings")) {
    success = wifiManager.setCredentials(json);
    if(success) wifiManager.connect();
  }

  return(success);
}

void printStatus() {
  switch (currentStatus) {
    case YoYoWiFiManager::YY_CONNECTED:
      Serial.println("YY_CONNECTED");
      break;
    case YoYoWiFiManager::YY_IDLE_STATUS:
      Serial.println("YY_IDLE_STATUS");
      break;
    case YoYoWiFiManager::YY_NO_SSID_AVAIL:
      Serial.println("YY_NO_SSID_AVAIL");
      break;
    case YoYoWiFiManager::YY_SCAN_COMPLETED:
      Serial.println("YY_SCAN_COMPLETED");
      break;
    case YoYoWiFiManager::YY_CONNECT_FAILED:
      Serial.println("YY_CONNECT_FAILED");
      break;
    case YoYoWiFiManager::YY_CONNECTION_LOST:
      Serial.println("YY_CONNECTION_LOST");
      break;
    case YoYoWiFiManager::YY_DISCONNECTED:
      Serial.println("YY_DISCONNECTED");
      break;
    case YoYoWiFiManager::YY_CONNECTED_PEER_CLIENT:
      Serial.println("YY_CONNECTED_PEER_CLIENT");
      break;
    case YoYoWiFiManager::YY_CONNECTED_PEER_SERVER:
      Serial.println("YY_CONNECTED_PEER_SERVER");
      break;
  }   
}

// Generates a unique ID based on the ESP32's mac
String generateID() {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}