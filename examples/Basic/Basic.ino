#include "YoYoWiFiManager.h"
#include "Settings.h"

YoYoWiFiManager wifiManager;
Settings *settings;

void setup() {
  Serial.begin(115200);

  settings = new Settings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings);

  wifiManager.begin("YoYoMachines", "blinkblink");
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  if(wifiStatus == YY_CONNECTED) {
    //YY_CONNECTED is equal to WL_CONNECTED
  }
}