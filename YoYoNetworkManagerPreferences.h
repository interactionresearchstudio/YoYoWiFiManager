#ifndef YoYoNetworkManagerPreferences_h
#define YoYoNetworkManagerPreferences_h

#include <Preferences.h>

class YoYoNetworkManagerPreferences
{
    private:
        Preferences preferences;
    public:
        bool begin(const char * name, bool readOnly=false) {
            return(begin(name, readOnly));
        }

        void end() {
            preferences.end();
        }

        String getString(const char* key, String defaultValue = String()) {
            return(preferences.getString(key, defaultValue));
        }

        size_t putString(const char* key, String value) {
            return(preferences.putString(key, value));
        }
        
        bool getBool(const char* key, bool defaultValue = false) {
            return(preferences.putBool(key, defaultValue));
        }

        size_t putBool(const char* key, bool value) {
            return(preferences.putBool(key, value));
        }

        bool clear() {
            return(preferences.clear());
        }
};

#endif