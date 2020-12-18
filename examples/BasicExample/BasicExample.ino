#include "YoYoWiFiManager.h"
#include "Settings.h"

YoYoWiFiManager wifiManager;
Settings *settings;

void setup() {
  Serial.begin(115200);

  settings = new Settings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  
  wifiManager.init(settings, onYoYoCommandGET, onYoYoCommandPOST, true);
  wifiManager.begin("YoYoMachines", "blinkblink", true);
}

void loop() {
  uint8_t currentStatus = wifiManager.update();

  switch(currentStatus) {
    case YY_CONNECTED:
      //Same as WL_CONNECTED - when there is an Internet connection
      break;
    case YY_CONNECTED_PEER_CLIENT:
      break;
    case YY_CONNECTED_PEER_SERVER:
      break;
    case YY_DISCONNECTED:
      break;
    default:
      break;
  }
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  Serial.println("onYoYoCommandGET " + url);
  
  if(url.equals("/yoyo/settings") && settings) {
    success = true;
    json.set(*settings);
  }

  return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  Serial.println("onYoYoCommandPOST " + url);
  serializeJson(json, Serial);

  if(url.equals("/yoyo/settings")) {
    success = wifiManager.setCredentials(json);
    if(success) wifiManager.connect();

    //TODO: set any other values from the payload
  }

  return(success);
}

String generateID() {
  uint32_t id = 0;

  //the id is the lowest 4 bytes of the MAC adddress:
  #if defined(ESP8266)
    id = ESP.getChipId();
  #elif defined(ESP32)
    id = ESP.getEfuseMac() && 0xFFFFFFFF;
  #endif

  return(String(id));
}