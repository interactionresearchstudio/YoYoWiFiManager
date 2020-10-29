#include "YoYoNetworkManager.h"

YoYoNetworkManager nManager;

void setup() {
  Serial.begin(115200);
  
  nManager.begin();
}

void loop() {
  nManager.update();
}
