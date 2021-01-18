#ifndef YoYoNetworkSettingsInterface_h
#define YoYoNetworkSettingsInterface_h

class YoYoNetworkSettingsInterface {
  public:
    virtual int getNumberOfNetworkCredentials() = 0;
    virtual bool addNetwork(const char *ssid, const char *password, bool autosave = true) = 0;
    virtual void getSSID(int n, char *ssid) = 0;
    virtual void getPassword(int n, char *password) = 0;
    virtual int getNetwork(const char *ssid) = 0;

    virtual bool removeNetwork(const char *ssid, bool autosave = true) = 0;
    virtual void clearNetworks(bool autosave = true) = 0;

    virtual void setLastNetwork(const char *ssid, bool autosave = true) = 0;
    virtual int getLastNetwork() = 0;

    virtual bool isFull() = 0;

    bool hasNetworkCredentials() {
      return(getNumberOfNetworkCredentials() > 0);
    }
};

#endif