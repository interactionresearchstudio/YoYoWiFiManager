#ifndef YoYoWiFiManagerSettings_h
#define YoYoWiFiManagerSettings_h

class YoYoWiFiManagerSettings {
  public:
    virtual bool hasNetworkCredentials() = 0;
    virtual int getNumberOfNetworkCredentials() = 0;
    virtual void addNetwork(const char *ssid, const char *password) = 0;
    virtual void getSSID(int n, char *ssid) = 0;
    virtual void getPassword(int n, char *password) = 0;
};

#endif