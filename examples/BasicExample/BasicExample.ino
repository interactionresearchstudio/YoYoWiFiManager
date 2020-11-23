#include "YoYoWiFiManager.h"
#include "Settings.h"

YoYoWiFiManager *wifiManager;
Settings settings;

void setup() {
  Serial.begin(115200);
  Serial.println("HELLO");

  wifiManager = new YoYoWiFiManager();
  wifiManager -> autoConnect("YoYoMachines", "blinkblink");
  /*
  //loadCredentials();
  //setPairedStatus();
  myID = generateID();

  if (wifiCredentials == "" || getNumberOfMacAddresses() < 2) {
    wifiManager.autoConnect("YoYoMachines", "blinkblink"); //blocking
  }
  else {
    Serial.print("List of Mac addresses:");
    Serial.println(macCredentials);
    //connect to router to talk to server
    digitalWrite(wifiLEDPin, 0);
    connectToWifi(wifiCredentials);
    //checkForUpdate();
    currentSetupStatus = setup_finished;
    Serial.println("setup complete");
  }
  */

  // if(WiFi.status() == WL_CONNECTED) {
  //   //connected
  // }
}

void loop() {
}

// Generates a unique ID based on the ESP32's mac
String generateID() {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}