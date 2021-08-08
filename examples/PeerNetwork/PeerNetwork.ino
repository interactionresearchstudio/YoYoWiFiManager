/*
    Example documented here > https://github.com/interactionresearchstudio/YoYoWiFiManager#peernetwork
*/

#include <YoYoWiFiManager.h>
YoYoWiFiManager wifiManager;

void setup() {
  Serial.begin(115200);

  wifiManager.init();

  //Attempt to connect to a WiFi network previously saved in the settings, 
  //if one can not be found start a captive portal called "YoYoMachines", 
  //with a password of "blinkblink" to configure a new one:
  wifiManager.begin("YoYoMachines", "blinkblink", false);
}

void loop() {
  wifiManager.loop();
}
