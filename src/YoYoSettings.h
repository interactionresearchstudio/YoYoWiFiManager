#ifndef YoYoSettings_h
#define YoYoSettings_h

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>

#if defined(ESP8266)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#elif defined(ESP32)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#endif

class YoYoSettings : public DynamicJsonDocument, public YoYoNetworkSettingsInterface {
    private:
        int eepromAddress = 0;
        int eepromCapacityBytes = 0;

        void init(int eepromCapacityBytes, int eepromAddress) {
            this -> eepromAddress = eepromAddress;
            this -> eepromCapacityBytes = min(YY_MAX_EEPROM_CAPACITY_BYTES - eepromAddress, eepromCapacityBytes);;

            EEPROM.begin(this -> eepromCapacityBytes);
            EepromStream eepromStream(this -> eepromAddress, this -> eepromCapacityBytes);
            deserializeJson(*this, eepromStream);
        }

    public:
        YoYoSettings(int capacityBytes, int address = 0) : DynamicJsonDocument(capacityBytes) {
            Serial.println("Settings");
            init(capacityBytes, address);
        }

        int getNumberOfNetworkCredentials() {
            int result = (*this)["credentials"].size();

            return(result);
        }

        void addNetwork(const char *ssid, const char *password, bool autosave = true) {
            Serial.printf("Settings::addNetwork %s  %s\n", ssid, password);

            StaticJsonDocument<128> json;
            json["ssid"] = ssid;
            json["password"] = password;

            Serial.print("Adding... ");
            serializeJson(json, Serial);
            Serial.print("\n");

            (*this)["credentials"].add(json);

            if(autosave) save();
        }

        void getSSID(int n, char *ssid) {
            strcpy(ssid, (*this)["credentials"][n]["ssid"]);
        }

        void getPassword(int n, char *password) {
            strcpy(password, (*this)["credentials"][n]["password"]);
        }

        bool save() {
            Serial.println("SAVE---SAVE---SAVE---SAVE---SAVE");
            serializeJson(*this, Serial);

            EepromStream eepromStream(this -> eepromAddress, this -> eepromCapacityBytes);
            serializeJson(*this, eepromStream);
            eepromStream.flush(); 

            return(true);
        }
};

#endif