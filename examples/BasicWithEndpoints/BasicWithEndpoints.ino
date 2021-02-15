/*
    Example documented here > https://github.com/interactionresearchstudio/YoYoWiFiManager#basicwithendpoints
*/

#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, onceConnected, onYoYoMessageGET, onYoYoMessagePOST, true);

  wifiManager.begin("YoYoMachines", "blinkblink", true);
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

bool onYoYoMessageGET(JsonVariant message) {
  bool success = false;

  Serial.println("onYoYoCommandGET " + message["path"].as<String>());
  
  if(message["path"] == "/yoyo/settings" && settings) {
    message["payload"].set(*settings);
    success = true;
  }

  return(success);
}

bool onYoYoMessagePOST(JsonVariant message) {
  bool success = false;
  
  Serial.println("onYoYoCommandPOST " + message["path"].as<String>());
  serializeJson(message, Serial);

  //define an alternative to using the built-in /yoyo/credentials endpoint that also allows custom value to be set in the settings doc
  if(message["path"] == "/yoyo/settings") {
    //TODO: merge settings doc with values from the payload
    //(*settings)["name"] = "value";
    
    success = wifiManager.setCredentials(message["payload"]);
    if(success) {
      (*settings).save();
      wifiManager.connect();
      message["broadcast"] = true;
    }
  }

  return(success);
}