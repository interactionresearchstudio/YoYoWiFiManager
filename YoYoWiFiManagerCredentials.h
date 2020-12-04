#ifndef YoYoWiFiManagerCredentials_h
#define YoYoWiFiManagerCredentials_h

#if defined(ESP8266)
#elif defined(ESP32)
    #include <Preferences.h>
#endif

#define YoYoWiFiManagerCredentialsNameSpace "YoYoCred"
#define YoYoWiFiManagerCredentialsListMax 8

class YoYoWiFiManagerCredentials {
    private:
        Preferences credentials;
        String *credentialsAsList[YoYoWiFiManagerCredentialsListMax];

        void init() {
            for(int n=0; n < YoYoWiFiManagerCredentialsListMax; ++n) {
                credentialsAsList[n] = NULL;
            }
            
            loadCredentials();
        }

        size_t getString(const char* key, char* value, const size_t maxLen) {
            size_t len = 0;
            
            credentials.begin(YoYoWiFiManagerCredentialsNameSpace);
            len = credentials.getString(key, value, maxLen);
            credentials.end();

            return(len);
        }

        void putString(const char* key, String value) {
            credentials.begin(YoYoWiFiManagerCredentialsNameSpace);
            credentials.putString(key, value);
            credentials.end();
        }

        void loadCredentials() {
            char s[256];
            if(getString("credentials", s, 256) > 0) {
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
            String s;
            for(int n=0; n < YoYoWiFiManagerCredentialsListMax; ++n) {
                if(credentialsAsList[n] != NULL) {
                    s+=*credentialsAsList[n];
                    s+=":";
                }
            }
            Serial.printf("saveCredentials: %s\n", s.c_str());
            putString("credentials", s);
        }
    public:
        YoYoWiFiManagerCredentials() {
            Serial.println("YoYoWiFiManagerCredentials");
            init();
        }

        bool available() {
            return(getQuantity() > 0);
        }

        void add(const char *ssid, const char *password) {
            int oldestN = YoYoWiFiManagerCredentialsListMax-1;
            if(credentialsAsList[oldestN] != NULL) delete credentialsAsList[oldestN];

            for(int n = oldestN; n > 0; --n) {
                credentialsAsList[n] = credentialsAsList[n-1];
            }
            credentialsAsList[0] = new String(String(ssid) + "," + String(password));   //TODO: double check seprator chars can be relied on;

            saveCredentials();
        }

        String *get(int n) {
            String *result = NULL;
            
            if(n >= 0 && n < YoYoWiFiManagerCredentialsListMax) {
                result = credentialsAsList[n];
            }

            return(result);
        }

        String *getSSID(int n) {
            String *result = NULL;

            if(n >= 0 && n < YoYoWiFiManagerCredentialsListMax) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(0, s -> indexOf(',')));  //TODO: memory leak
            }

            return(result);
        }

        String *getPassword(int n) {
            String *result = NULL;

            if(n >= 0 && n < YoYoWiFiManagerCredentialsListMax) {
                String *s = credentialsAsList[n];
                result = new String(s -> substring(s -> indexOf(',')+1));  //TODO: memory leak
            }

            return(result);
        }

        int getQuantity() {
            int result = YoYoWiFiManagerCredentialsListMax;

            for(int n=0; n < YoYoWiFiManagerCredentialsListMax && result == YoYoWiFiManagerCredentialsListMax; ++n) {
                if(credentialsAsList[n] == NULL) {
                    result = n;
                }
            }

            return(result);
        }

        bool clear() {
            bool result = false;

            credentials.begin(YoYoWiFiManagerCredentialsNameSpace);
            result = credentials.clear();
            credentials.end();

            return(result);
        }
};

#endif