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

        bool addNetwork(const char *ssid, const char *password, bool autosave = true) {
            bool success = false;
            
            Serial.printf("Settings::addNetwork %s  %s\n", ssid, password);

            int index = getNetwork(ssid);
            if(index >= 0) {
                //Rewrite the password of an existing network:
                (*this)["credentials"][index]["password"] = password;
            }
            else {
                //Add a new network:
                StaticJsonDocument<128> json;
                json["ssid"] = ssid;
                json["password"] = password;

                (*this)["credentials"].add(json);
            }

            if(autosave) save();

            return(success);
        }

        int getNetwork(const char *ssid) {
            int index = -1;

            for(int n = 0; n < (*this)["credentials"].size(); ++n) {
                if((*this)["credentials"][n]["ssid"] == ssid) {
                    index = n;
                    break;
                }
            }

            return(index);
        }

        void getSSID(int n, char *ssid) {
            strcpy(ssid, (*this)["credentials"][n]["ssid"]);
        }

        void getPassword(int n, char *password) {
            strcpy(password, (*this)["credentials"][n]["password"]);
        }

        void setLastNetwork(const char *ssid, bool autosave) {
            int index = getNetwork(ssid);

            if(index >= 0) {
                for(int n = 0; n < (*this)["credentials"].size(); ++n) {
                    JsonVariant network = (*this)["credentials"][n];

                    if(n == index) {
                        network["lastnetwork"] = true;
                    }
                    else {
                        if(network["lastnetwork"]) {
                            network.remove("lastnetwork");
                        }
                    }
                }
                if(autosave) save();
            }
        }

        int getLastNetwork() {
            int index = -1;

            for(int n = 0; n < (*this)["credentials"].size(); ++n) {
                if((*this)["credentials"][n]["lastnetwork"]) {
                    index = n;
                    break;
                }
            }

            return(index);
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