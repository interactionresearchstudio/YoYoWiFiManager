#include "YoYoWiFiManager.h"
#include "Settings.h"

YoYoWiFiManager *wifiManager;
Settings settings;

void setup() {
  Serial.begin(115200);
  Serial.println("HELLO");

  wifiManager = new YoYoWiFiManager(onYoYoCommandGET, onYoYoCommandPOST);
  wifiManager -> begin("YoYoMachines", "blinkblink", false);
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
  if(wifiManager != NULL) {
     wifiManager -> update();
  }
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  //put the results of this into the json object
  Serial.println("onYoYoCommandGET " + url);
  
  /*
  Serial.println("getSettings");

  String local_ssid = json["local_ssid"].as<String>();
  String local_pass = json["local_pass"].as<String>();
  String remote_ssid = json["remote_ssid"].as<String>();
  String remote_pass = json["remote_pass"].as<String>();
  String remote_mac = json["remote_mac"].as<String>();

  if (remote_mac != "") {
    //TODO:
    //addToMacAddressJSON(remote_mac);
    success = true;
  }

  if (remote_pass != "" && remote_ssid != "" && local_ssid != "" && local_pass != "") {
    //TODO:
    //addToWiFiJSON(local_ssid, local_pass);
    //addToWiFiJSON(remote_ssid, remote_pass);
    //sendWifiCredentials();
    success = true;
  }
  else if (local_pass != "" && local_ssid != "" && remote_ssid == "" && remote_pass == "") {
    //TODO:
    //addToWiFiJSON(local_ssid, local_pass);
    //sendWifiCredentials();
    success = true;
  }
  */

 return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  //read this json object and do something
  Serial.println("onYoYoCommandPOST " + url);

  //TODO:
  /*
  settingsJsonDoc["local_mac"] = myID;
  settingsJsonDoc["local_ssid"] = "";
  settingsJsonDoc["local_pass_len"] = 0; //local_pass.length;
  settingsJsonDoc["remote_ssid"] = "";
  settingsJsonDoc["remote_pass_len"] = 0; //remote_pass.length;
  settingsJsonDoc["remote_mac"] = getRemoteMacAddress(1);
  settingsJsonDoc["local_paired_status"] = getCurrentPairedStatusAsString();
  Serial.println(getCurrentPairedStatusAsString());
  */

  return(success);
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