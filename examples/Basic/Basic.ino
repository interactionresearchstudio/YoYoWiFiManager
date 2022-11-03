/*
    Example documented here > https://github.com/interactionresearchstudio/YoYoWiFiManager#basic
*/

#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, onceConnected);

  (*settings)["random"] = random(0,100);
  serializeJson((*settings), Serial);
  Serial.println();
  (*settings).save();

  wifiManager.begin("YoYoMachines", "blinkblink");
}

void onceConnected() {
  //When status changes to YY_CONNECTED - loop() must be being called
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  if(wifiStatus == YY_CONNECTED) {
    //YY_CONNECTED is equal to WL_CONNECTED
  }
}