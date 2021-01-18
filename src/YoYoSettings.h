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
                success = true;
            }
            else {
                //Add a new network:
                JsonObject network  = (*this)["credentials"].createNestedObject();
                success = !network.isNull() && (network["ssid"] = ssid) && (network["password"] = password);
            }

            if(autosave && success) save();

            return(success);
        }

        bool removeNetwork(const char *ssid, bool autosave = true) {
            bool success = false;
            
            JsonVariant credentials = (*this)["credentials"];
            for (JsonObject::iterator it = credentials.as<JsonObject>().begin(); it != credentials.as<JsonObject>().end(); ++it) {
                if (it->value()["ssid"] == ssid) {
                    credentials.as<JsonObject>().remove(it);
                    success = true;
                }
            }

            if(autosave && success) save();

            return(success);
        }

        void clearNetworks(bool autosave = true) {
            (*this)["credentials"].clear();

            if(autosave) save();
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
            String s = (*this)["credentials"][n]["ssid"];
            if(s) strcpy(ssid, s.c_str());
        }

        void getPassword(int n, char *password) {
            String s = (*this)["credentials"][n]["password"];
            if(s) strcpy(password, s.c_str());
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
            bool success = false;

            Serial.println("SAVE---SAVE---SAVE---SAVE---SAVE");
            serializeJson(*this, Serial);

            EepromStream eepromStream(this -> eepromAddress, this -> eepromCapacityBytes);
            serializeJson(*this, eepromStream);
            eepromStream.flush(); 
            success = true;

            return(success);
        }

        bool isFull() {
            bool result = false;

            //TODO: implement - measure if there's enough space for a new network

            return(result);
        }
};

#endif