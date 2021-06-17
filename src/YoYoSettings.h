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

        bool addNetwork(const char *ssid, const char *password, bool force = false, bool autosave = true) {
            bool success = false;
            
            Serial.printf("Settings::addNetwork %s  %s\n", ssid, password);

            int index = getNetwork(ssid);
            if(index >= 0) {
                //Rewrite the password of an existing network - NB password must be (char *) not (const char *) otherwise only the pointer is copied:
                success =  setNetwork((*this)["credentials"][index], NULL, password);
            }
            else {
                //Add a new network:
                if(ssid && password) {
                    while(force && isFull()) removeNetwork(0, false);   //if storage is full - remove the oldest network - TODO: should it delete this network if lastnetwork is set true?

                    success =  setNetwork((*this)["credentials"].createNestedObject(), ssid, password);
                }
            }

            if(autosave && success) save();

            return(success);
        }

        bool setNetwork(JsonVariant network, const char *ssid, const char *password) {
            bool success = false;

            if(!network.isNull()) {
                //NB ssid and password must be (char *) not (const char *) otherwise only the pointer is copied
                if(ssid) network["ssid"] = (char *) ssid;
                if(password) network["password"] = (char *) password;

                success = true;
            }

            return(success);
        }

        bool removeNetwork(int index, bool autosave = true) {
            bool success = false;

            JsonArray credentials = (*this)["credentials"];
            if(index >= 0 && index < credentials.size()) {
                credentials.remove(index);
                garbageCollect();

                success = true;
            }

            if(autosave && success) save();

            return(success);
        }

        bool removeNetwork(const char *ssid, bool autosave = true) {
            bool success = removeNetwork(getNetwork(ssid), autosave);

            if(autosave && success) save();

            return(success);
        }

        void clearNetworks(bool autosave = true) {
            (*this)["credentials"].clear();
            garbageCollect();

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

        bool getSSID(int index, char *ssid) {
            bool success = false;

            if(index < (*this)["credentials"].size() && ssid) {
                ssid[0] = '\0';
                const char *v = (*this)["credentials"][index]["ssid"];
                if(v) {
                    strcpy(ssid, v);
                    success = true;
                }
            }

            return(success);
        }

        bool getPassword(int index, char *password) {
            bool success = false;

            if(index < (*this)["credentials"].size() && password) {
                password[0] = '\0';
                const char *v = (*this)["credentials"][index]["password"];
                if(v) {
                    strcpy(password, v);
                    success = true;
                }
            }

            return(success);
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
            Serial.println();

            EepromStream eepromStream(this -> eepromAddress, this -> eepromCapacityBytes);
            serializeJson(*this, eepromStream);
            eepromStream.flush(); 
            success = true;

            return(success);
        }

        bool isFull() {
            int freeBytes = capacity() - memoryUsage();
            int networkBudgetBytes = (SSID_MAX_LENGTH + PASSWORD_MAX_LENGTH + 64);  //64 is the budget for the json notation

            return(networkBudgetBytes > freeBytes);
        }
};

#endif