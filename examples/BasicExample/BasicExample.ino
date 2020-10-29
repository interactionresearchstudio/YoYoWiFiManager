#include "YoYoNetworkManager.h"

YoYoNetworkManager nManager;

void setup() {
  Serial.begin(115200);

  nManager.begin(); //blocking

  // if(WiFi.status() == WL_CONNECTED) {
  //   //connected
  // }
}

void loop() {
  nManager.update();
}