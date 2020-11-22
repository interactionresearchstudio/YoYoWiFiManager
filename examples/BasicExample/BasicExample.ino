#include "YoYoWiFiManager.h"

YoYoWiFiManager wifiManager;

void setup() {
  Serial.begin(115200);

  wifiManager.autoConnect("YoYoMachines", "blinkblink"); //blocking

  // if(WiFi.status() == WL_CONNECTED) {
  //   //connected
  // }
}

void loop() {
}