#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>
#include <AceButton.h>
using namespace ace_button;

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

const int LED_GREEN_PIN = 12;
const int LED_RED_PIN = 13;
const int LED_BLUE_PIN = 14;
const int BUTTON_PIN = 15;

AceButton button(BUTTON_PIN);

int red = 0, green = 0, blue = 0;

void handleButtonEvent(AceButton* /* button */, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      Serial.println("AceButton::kEventPressed");
      onButtonPressed();
      break;
    case AceButton::kEventReleased:
      Serial.println("AceButton::kEventReleased");
      break;
  }
}

void onButtonPressed() {
  //TODO add some interaction
}

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, NULL, onYoYoMessageGET, onYoYoMessagePOST, true);
  wifiManager.begin("YoYoMachines", "blinkblink", false);
  
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT);
  button.init(BUTTON_PIN, LOW);
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);
}

void loop() {
  wifiManager.loop();
  button.check();
}

bool onYoYoMessageGET(JsonVariant message) {
  bool success = false;

  if(message["path"] == "/yoyo/colour") {
    message["payload"]["red"] = red;
    message["payload"]["green"] = green;
    message["payload"]["blue"] = blue;

    success = true;
  }

  return(success);
}

bool onYoYoMessagePOST(JsonVariant message) {
  bool success = false;
  
  if(message["path"] == "/yoyo/colour") {
    red = message["payload"]["red"].as<int>();
    green = message["payload"]["green"].as<int>();
    blue = message["payload"]["blue"].as<int>();
    updateLEDColourRGB();

    message["broadcast"] = true;
    success = true;
  }

  return(success);
}

void updateLEDColourRGB() {
  #if defined(ESP8266)
    analogWrite(LED_RED_PIN, red);
    analogWrite(LED_GREEN_PIN, green);
    analogWrite(LED_BLUE_PIN, blue);

  #elif defined(ESP32)
    //TODO: analogWrite is not defined for ESP32

  #endif
}