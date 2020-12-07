#ifndef YoYoWiFiManagerSetings_h
#define YoYoWiFiManagerSetings_h

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>

// #define YoYoWiFiManagerSetingsAddress       0
// #define YoYoWiFiManagerSetingsSize          EEPROM.length()

// #define YoYoWiFiManagerSetingsMaxListCount  8
//           512

#if defined(ESP8266)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#elif defined(ESP32)
    #define YY_MAX_EEPROM_CAPACITY_BYTES  512
#endif

class YoYoWiFiManagerSetings : public DynamicJsonDocument {
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
        YoYoWiFiManagerSetings(int capacityBytes, int address = 0) : DynamicJsonDocument(capacityBytes) {
            Serial.println("YoYoWiFiManagerSetings");
            init(capacityBytes, address);
        }

        bool hasNetworkCredentials() {
            return(getNumberOfNetworkCredentials() > 0);
        }

        int getNumberOfNetworkCredentials() {
            int result = (*this)["credentials"].size();

            return(result);
        }

        void addNetwork(const char *ssid, const char *password) {
            // int oldestN = YoYoWiFiManagerSetingsListMax-1;
            // if(credentialsAsList[oldestN] != NULL) delete credentialsAsList[oldestN];

            // for(int n = oldestN; n > 0; --n) {
            //     credentialsAsList[n] = credentialsAsList[n-1];
            // }
            // credentialsAsList[0] = new String(String(ssid) + "," + String(password));   //TODO: double check seprator chars can be relied on;

            // saveCredentials();
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
            
            if(n >= 0 && n < YoYoWiFiManagerSetingsMaxListCount) {
                result = credentialsAsList[n];
            }

            return(result);
        }

        String *getSSID(int n) {
            String *result = NULL;

            if(n >= 0 && n < YoYoWiFiManagerSetingsMaxListCount) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(0, s -> indexOf(',')));  //TODO: memory leak
            }

            return(result);
        }

        String *getPassword(int n) {
            String *result = NULL;

            if(n >= 0 && n < YoYoWiFiManagerSetingsMaxListCount) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(s -> indexOf(',')+1));  //TODO: memory leak
            }

            return(result);
        }

        void clear() {
            for (int i = YoYoWiFiManagerSetingsAddress; i < YoYoWiFiManagerSetingsSize; i++) {
                EEPROM.write(i, 0);
            }
        }
        */
};

#endif