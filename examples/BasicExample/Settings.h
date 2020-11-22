#ifndef Settings_h
#define Settings_h

class Settings {
    /*
    void YoYoWiFiManager::loadCredentials() {
        preferences.begin("scads", false);
        wifiCredentials = preferences.getString("wifi", "");
        macCredentials = preferences.getString("mac", "");
        preferences.end();
    }

    void YoYoWiFiManager::setPairedStatus() {
        int numberOfMacAddresses = getNumberOfMacAddresses();
        if (numberOfMacAddresses == 0) {
            Serial.println("setting up JSON database for mac addresses");
            preferences.clear();
            addToMacAddressJSON(myID);
        }
        else if (numberOfMacAddresses < 2) {
            //check it has a paired mac address
            Serial.println("Already have local mac address in preferences, but nothing else");
        }
        else {
            currentPairedStatus = pairedSetup;
            Serial.println("Already has one or more paired mac address");
        }
    }

    int YoYoWiFiManager::getNumberOfMacAddresses() {
        int numberOfMacAddresses = 0;

        //Returns the number of mac address in JSON array
        preferences.begin("scads", false);
        String macCredentials = preferences.getString("mac", "");
        preferences.end();

        if (macCredentials != "") {
            const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
            DynamicJsonDocument addresses(capacity);
            deserializeJson(addresses, macCredentials);
            numberOfMacAddresses = addresses["mac"].size();
        }

        return (numberOfMacAddresses);
    }

    void YoYoWiFiManager::addToMacAddressJSON(String addr) {
        // appends mac address to memory json array if isn't already in it, creates the json array if it doesnt exist
        preferences.begin("scads", false);
        String macAddressList = preferences.getString("mac", "");
        if (macAddressList != "") {
            const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
            DynamicJsonDocument addresses(capacity);
            deserializeJson(addresses, macAddressList);
            JsonArray mac = addresses["mac"];
            inList = false;
            for ( int i = 0; i < mac.size(); i++) {
            if (mac[i] == addr) {
                inList = true;
                Serial.println("mac address already in list");
                break;
            }
            }
            if (inList == false) {
            mac.add(addr);
            Serial.print("adding ");
            Serial.print(addr);
            Serial.println(" to the address list");
            macAddressList = "";
            serializeJson(addresses, macAddressList);
            Serial.println(macAddressList);
            preferences.putString("mac", macAddressList);
            }
        } else {
            const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 10;
            DynamicJsonDocument addresses(capacity);
            JsonArray macArray = addresses.createNestedArray("mac");
            macArray.add(addr);
            macAddressList = "";
            serializeJson(addresses, macAddressList);
            preferences.putString("mac", macAddressList);
            Serial.print("creating json object and adding the local mac ");
            Serial.print(addr);
            Serial.println(" to the address list");
        }
        preferences.end();
    }
    */
};

#endif