#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

const int LED_RED_PIN = 13;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, NULL, onYoYoCommandGET, onYoYoCommandPOST, true);
  wifiManager.begin("YoYoMachines", "blinkblink", false);
  
  pinMode(LED_RED_PIN, OUTPUT);
}

void loop() {
  wifiManager.loop();
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  return(false);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  if(url.equals("/yoyo/active")) {
    digitalWrite(LED_RED_PIN, json["active"].as<bool>());
    success = true;
  }

  return(success);
}