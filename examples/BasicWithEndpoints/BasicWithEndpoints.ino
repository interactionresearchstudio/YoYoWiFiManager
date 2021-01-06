#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, onceConnected, onYoYoCommandGET, onYoYoCommandPOST, true);

  wifiManager.begin("YoYoMachines", "blinkblink");
}

void onceConnected() {
  //When status changes to YY_CONNECTED - loop() must be being called
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  switch(wifiStatus) {
    case YY_NO_SHIELD:
      break;
    case YY_IDLE_STATUS:
      break;
    case YY_NO_SSID_AVAIL:
      break;
    case YY_SCAN_COMPLETED:
      break;
    case YY_CONNECTED:
      break;
    case YY_CONNECT_FAILED:
      break;
    case YY_CONNECTION_LOST:
      break;
    case YY_DISCONNECTED:
      break;
    case YY_CONNECTED_PEER_CLIENT:
      break;
    case YY_CONNECTED_PEER_SERVER:
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
    //an alternative to using the built-in /yoyo/credentials...
    success = wifiManager.setCredentials(json);
    if(success) wifiManager.connect();

    //TODO: set any other values from the payload
  }

  return(success);
}