#ifndef WIFI_CREDENTIAL_MANAGER
#define WIFI_CREDENTIAL_MANAGER

#include <Arduino.h>

#define MAX_WIFI_CREDENTIALS 5

class WiFiCredentialManager {
public:
    WiFiCredentialManager();
    void load();
    void save();
    void add(String ssid, String password);
    bool deleteSsid(String ssid);
    String getSsid(int i);
    String getPassword(int i);
    int count();
private:
    int _count;
    class WiFiCredential {
    public:
        String ssid;
        String password;
    };
    WiFiCredential credentials[MAX_WIFI_CREDENTIALS];
};

#endif //WIFI_CREDENTIAL_MANAGER