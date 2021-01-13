#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

const int LED_RED_PIN = 13;
const int LED_GREEN_PIN = 12;
const int LED_BLUE_PIN = 14;

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, NULL, onYoYoCommandGET, onYoYoCommandPOST, true);
  wifiManager.begin("YoYoMachines", "blinkblink", false);
  
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
}

void loop() {
  wifiManager.loop();
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  if(url.equals("/yoyo/colour")) {
    serializeJson(json, Serial);
    Serial.println();

    int red = json["red"].as<int>();
    int green = json["green"].as<int>();
    int blue = json["blue"].as<int>();

    Serial.printf("R:%i\tG:%i\tB:%i\n", red, green, blue);
    setLEDColourRGB(red, green, blue);

    wifiManager.broadcastToPeersPOST(url, json);

    success = true;
  }

  return(success);
}

void setLEDColourRGB(int r, int g, int b) {
  #if defined(ESP8266)
    analogWrite(LED_RED_PIN, r);
    analogWrite(LED_GREEN_PIN, g);
    analogWrite(LED_BLUE_PIN, b);

  #elif defined(ESP32)
    //analogWrite is not defined for ESP32

  #endif
}
