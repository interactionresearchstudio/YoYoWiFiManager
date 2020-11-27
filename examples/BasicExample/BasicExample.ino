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

/*
      YY_NO_SHIELD        = WL_NO_SHIELD,
      YY_IDLE_STATUS      = WL_IDLE_STATUS,
      YY_NO_SSID_AVAIL    = WL_NO_SSID_AVAIL,
      YY_SCAN_COMPLETED   = WL_SCAN_COMPLETED,
      YY_CONNECTED        = WL_CONNECTED,
      YY_CONNECT_FAILED   = WL_CONNECT_FAILED,
      YY_CONNECTION_LOST  = WL_CONNECTION_LOST,
      YY_DISCONNECTED     = WL_DISCONNECTED,
      //yo-yo specific:
      YY_CONNECTED_PEER_CLIENT,
      YY_CONNECTED_PEER_SERVER
*/

void loop() {
  if(wifiManager != NULL) {
     wifiManager -> update();
  }
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  //put the results of this into the json object
  Serial.println("onYoYoCommandGET " + url);
  
  if(url.equals("/yoyo/settings")) {
    success = true;
    json["payload"] = "hey! hey!";
  }

 return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  //read this json object and do something
  Serial.println("onYoYoCommandPOST " + url);

  if(url.equals("/yoyo/settings")) {
    success = true;
    serializeJson(json, Serial);
  }

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

// Generates a unique ID based on the ESP32's mac
String generateID() {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}