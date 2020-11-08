#include "YoYoWifi.h"

void YoYoWifi::attachLed(uint8_t pin) {
  this->wifiLEDPin = pin;
}

bool YoYoWifi::isConnected() {
  return !disconnected;
}

// Generates a unique ID based on the ESP32's mac
String YoYoWifi::generateID() {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}

// Creates Access Point for other device to connect to
void YoYoWifi::createSCADSAP() {
  scads_ssid = "Yo-Yo-" + generateID();
  Serial.print("Wifi name:");
  Serial.println(scads_ssid);

  WiFi.mode(WIFI_AP);
  delay(2000);

  WiFi.persistent(false);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(scads_ssid.c_str(), scads_pass.c_str());
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
}

// Scan and connect to local scads wifi. Returns true if a local scads has been
// found.
boolean YoYoWifi::scanAndConnectToLocalSCADS() {
  boolean foundLocalSCADS = false;

  // WiFi.scanNetworks will return the number of networks found
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        scads_ssid = WiFi.SSID(i);
        if (scads_ssid.indexOf("Yo-Yo-") > -1) {
          Serial.println("Found YOYO");
          foundLocalSCADS = true;
          wifiMulti.addAP(scads_ssid.c_str(), scads_pass.c_str());
          while ((wifiMulti.run() != WL_CONNECTED)) {
            delay(500);
            Serial.print(".");
          }
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
  }
  return (foundLocalSCADS);
}

void YoYoWifi::connectToWifi(String credentials) {

  String _wifiCredentials = credentials;
  const size_t capacity = 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(2) + 150;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, _wifiCredentials);
  JsonArray ssid = doc["ssid"];
  JsonArray pass = doc["password"];
  if (ssid.size() > 0) {
    for (int i = 0; i < ssid.size(); i++) {
      if (isWifiValid(ssid[i])) {
      wifiMulti.addAP(checkSsidForSpelling(ssid[i]).c_str(), pass[i]);
      }
    }
  } else {
    Serial.println("issue with wifi credentials, creating access point");
  }

  Serial.println("Connecting to Router");

  long wifiMillis = millis();
  bool connectSuccess = false;

  preferences.begin("scads", false);
  bool hasConnected = preferences.getBool("hasConnected");
  preferences.end();

  while (!connectSuccess) {

    uint8_t currentStatus = wifiMulti.run();

    printWifiStatus(currentStatus);

    if (currentStatus == WL_CONNECTED) {
      // Connected!

      if (!hasConnected) {
        // TODO add this to preferences manager
        preferences.begin("scads", false);
        preferences.putBool("hasConnected", true);
        preferences.end();
        hasConnected = true;
      }
      connectSuccess = true;
      break;
    }

    if (millis() - wifiMillis > WIFICONNECTTIMEOUT) {
      // Timeout, check if we're out of range.
      while (hasConnected) {
        // Out of range, keep trying
        uint8_t _currentStatus = wifiMulti.run();
        if (_currentStatus == WL_CONNECTED) {
          digitalWrite(wifiLEDPin, 0);
          preferences.begin("scads", false);
          preferences.putBool("hasConnected", true);
          preferences.end();
          break;
        }
        else {
          digitalWrite(wifiLEDPin, 1);
          delay(100);
          Serial.print(".");
        }
        yield();
      }
      if (!hasConnected) {
        // Wipe credentials and reset
        Serial.println("Wifi connect failed, Please try your details again in the captive portal");
        // TODO add this to preferences manager
        preferences.begin("scads", false);
        preferences.putString("wifi", "");
        preferences.end();
        ESP.restart();
      }
    }

    delay(100);
    yield();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  disconnected = false;
}

String YoYoWifi::checkSsidForSpelling(String incomingSSID) {
  int n = WiFi.scanNetworks();
  int currMatch = 255;
  int prevMatch = currMatch;
  int matchID;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    Serial.println("can't find any wifi in the area");
    return incomingSSID;
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        currMatch = Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2;
        if (Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2) {
          if (currMatch < prevMatch) {
            prevMatch = currMatch;
            matchID = i;
          }
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
    if (prevMatch != 255) {
      Serial.println("Found a match!");
      return WiFi.SSID(matchID);
    } else {
      Serial.println("can't find any wifi that are close enough matches in the area");
      return incomingSSID;
    }
  }
}

void YoYoWifi::wifiCheck() {
  if (millis() - wificheckMillis > wifiCheckTime) {
    wificheckMillis = millis();
    if (wifiMulti.run() !=  WL_CONNECTED) {
      digitalWrite(wifiLEDPin, 1);
      disconnected = true;
    }
  }
}

bool YoYoWifi::isWifiValid(String incomingSSID) {
  int n = WiFi.scanNetworks();
  int currMatch = 255;
  int prevMatch = currMatch;
  int matchID;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    Serial.println("can't find any wifi in the area");
    return false;
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      String networkSSID = WiFi.SSID(i);
      if (networkSSID.length() <= SSID_MAX_LENGTH) {
        currMatch = Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2;
        if (Levenshtein::levenshteinIgnoreCase(incomingSSID.c_str(), WiFi.SSID(i).c_str()) < 2) {
          if (currMatch < prevMatch) {
            prevMatch = currMatch;
            matchID = i;
          }
        }
      } else {
        // SSID too long
        Serial.println("SSID too long for use with current ESP-IDF");
      }
    }
    if (prevMatch != 255) {
      Serial.println("Found a match!");
      return true;
    } else {
      Serial.println("can't find any wifi that are close enough matches in the area");
      return false;
    }
  }

}

void YoYoWifi::printWifiStatus(uint8_t status) {
  Serial.print("Status: ");
  switch (status) {
    case WL_CONNECTED:
      Serial.println("WL_CONNECTED");
      break;
    case WL_IDLE_STATUS:
      Serial.println("WL_IDLE_STATUS");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("WL_NO_SSID_AVAIL");
      break;
    case WL_SCAN_COMPLETED:
      Serial.println("WL_SCAN_COMPLETED");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("WL_CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("WL_CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      Serial.println("WL_DISCONNECTED");
      break;
  }
}
