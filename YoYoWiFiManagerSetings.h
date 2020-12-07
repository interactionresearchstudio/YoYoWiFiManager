#ifndef YoYoWiFiManagerSetings_h
#define YoYoWiFiManagerSetings_h

#include <EEPROM.h>

#define YoYoWiFiManagerSetingsMaxListCount  8
#define YoYoWiFiManagerSetingsAddress       0
#define YoYoWiFiManagerSetingsSize          512

class YoYoWiFiManagerSetings {
    private:
        //Preferences credentials;
        String *credentialsAsList[YoYoWiFiManagerSetingsMaxListCount];

        void init() {
            EEPROM.begin(YoYoWiFiManagerSetingsSize);

            for(int n=0; n < YoYoWiFiManagerSetingsMaxListCount; ++n) {
                credentialsAsList[n] = NULL;
            }

            loadCredentials();
        }

        size_t getString(const char* key, char* value, const size_t maxLen) {
            size_t len = 0;
            
            //credentials.begin(YoYoWiFiManagerSetingsNameSpace);
            //len = credentials.getString(key, value, maxLen);
            //credentials.end();

            return(len);
        }

        void putString(const char* key, String value) {
            // credentials.begin(YoYoWiFiManagerSetingsNameSpace);
            // credentials.putString(key, value);
            // credentials.end();
        }

        void loadCredentials() {
            Serial.println("loadCredentials");
            char s[YoYoWiFiManagerSetingsSize];

            EEPROM.get(YoYoWiFiManagerSetingsAddress, s);
            if(strlen(s) > 0) {
                const char d[2] = ":";

                char *credentials;
                credentials = strtok(s, d);
                
                Serial.printf("credentials %s\n", credentials);
                int n = 0;
                while(credentials != NULL) {
                    credentialsAsList[n] = new String(credentials);
                    
                    credentials = strtok(NULL, d);
                    ++n;
                }
            }
        }

        void saveCredentials() {
            /*
            String s;
            for(int n=0; n < YoYoWiFiManagerSetingsListMaxLength; ++n) {
                if(credentialsAsList[n] != NULL) {
                    s+=*credentialsAsList[n];
                    s+=":";
                }
            }
            Serial.printf("saveCredentials: %s\n", s.c_str());
            putString("credentials", s);
            */
        }

    public:
        YoYoWiFiManagerSetings() {
            Serial.println("YoYoWiFiManagerSetings");
            init();
        }

        bool available() {
            return(getQuantity() > 0);
        }

        void add(const char *ssid, const char *password) {
            /*
            int oldestN = YoYoWiFiManagerSetingsListMax-1;
            if(credentialsAsList[oldestN] != NULL) delete credentialsAsList[oldestN];

            for(int n = oldestN; n > 0; --n) {
                credentialsAsList[n] = credentialsAsList[n-1];
            }
            credentialsAsList[0] = new String(String(ssid) + "," + String(password));   //TODO: double check seprator chars can be relied on;

            saveCredentials();
            */
        }

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

        int getQuantity() {
            int result = YoYoWiFiManagerSetingsMaxListCount;

            // for(int n=0; n < YoYoWiFiManagerSetingsListMax && result == YoYoWiFiManagerSetingsListMax; ++n) {
            //     if(credentialsAsList[n] == NULL) {
            //         result = n;
            //     }
            // }

            return(result);
        }

        void clear() {
            for (int i = YoYoWiFiManagerSetingsAddress; i < YoYoWiFiManagerSetingsSize; i++) {
                EEPROM.write(i, 0);
            }
        }
};

#endif