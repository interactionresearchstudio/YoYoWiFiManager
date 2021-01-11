#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, NULL, onYoYoCommandGET, onYoYoCommandPOST, true);
  wifiManager.begin("YoYoMachines", "blinkblink", false);
}

void loop() {
  wifiManager.loop();
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  if(url.equals("/yoyo/colour")) {
    serializeJson(json, Serial);
    Serial.println();

    int red = json["red"].as<int>();
    int green = json["green"].as<int>();
    int blue = json["blue"].as<int>();

    Serial.printf("R:%i\tG:%i\tB:%i\n", red, green, blue);

    success = true;
  }

  return(success);
}
