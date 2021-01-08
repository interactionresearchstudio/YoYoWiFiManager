#include <YoYoWiFiManager.h>
YoYoWiFiManager wifiManager;

void setup() {
  Serial.begin(115200);

  wifiManager.init();
  wifiManager.begin("YoYoMachines", "blinkblink", false);
}

void loop() {
  wifiManager.loop();
}
