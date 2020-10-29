#include "YoYoNetworkManager.h"

YoYoNetworkManager nManager;

void setup() {
  nManager.begin();
}

void loop() {
  nManager.update();
}
