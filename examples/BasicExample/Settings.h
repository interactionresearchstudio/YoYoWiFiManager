#ifndef Settings_h
#define Settings_h

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>

// #define SettingsAddress       0
// #define SettingsSize          EEPROM.length()

// #define SettingsMaxListCount  8
//           512

#if defined(ESP8266)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#elif defined(ESP32)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#endif

class Settings : public DynamicJsonDocument, public YoYoWiFiManagerSettings {
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
        Settings(int capacityBytes, int address = 0) : DynamicJsonDocument(capacityBytes) {
            Serial.println("Settings");
            init(capacityBytes, address);
        }

        int getNumberOfNetworkCredentials() {
            int result = (*this)["credentials"].size();

            return(result);
        }

        void addNetwork(const char *ssid, const char *password) {
            JsonObject *json = new JsonObject();
            (*json)["ssid"] = ssid;
            (*json)["password"] = password;

            (*this)["credentials"].add(*json);

            delete json;
        }

        void getSSID(int n, char *ssid) {
            strcpy(ssid, (*this)["credentials"][n]["ssid"]);
        }

        void getPassword(int n, char *password) {
            strcpy(password, (*this)["credentials"][n]["password"]);
        }

        bool save() {
            EepromStream eepromStream(this -> eepromAddress, this -> eepromCapacityBytes);
            serializeJson(*this, eepromStream);
            eepromStream.flush(); 

            return(true);
        }

        /*
        String *get(int n) {
            String *result = NULL;
            
            if(n >= 0 && n < SettingsMaxListCount) {
                result = credentialsAsList[n];
            }

            return(result);
        }

        String *getSSID(int n) {
            String *result = NULL;

            if(n >= 0 && n < SettingsMaxListCount) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(0, s -> indexOf(',')));  //TODO: memory leak
            }

            return(result);
        }

        String *getPassword(int n) {
            String *result = NULL;

            if(n >= 0 && n < SettingsMaxListCount) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(s -> indexOf(',')+1));  //TODO: memory leak
            }

            return(result);
        }

        void clear() {
            for (int i = SettingsAddress; i < SettingsSize; i++) {
                EEPROM.write(i, 0);
            }
        }
        */
};

#endif