#ifndef WIFI_CONNECTION_MANAGER_H
#define WIFI_CONNECTION_MANAGER_H

#include <ESP8266WiFi.h>
#include <WiFiCredentialManager.h>
#include <IotLogger.h>

#define CONNECT_DELAY_MILLIS 30000

#define CONFIG_SSID "CSHIOT-%02X%02X%02X"
#define CONFIG_PASSWORD "iot-password"

#define CONFIG_AP_IP 0x0180A8C0
#define CONFIG_AP_MASK 0x00FFFFFF

class WiFiScanInfo {
public:
    String ssid;
    String bssidStr;
    uint8_t bssid[6];
    int rssi;
    uint8_t encType;
    int channel;
    bool hidden;

    WiFiScanInfo *next;

    ~WiFiScanInfo() { if (next) { delete next; } };
};

class WiFiConnectionManager {
public:
    WiFiConnectionManager();
    void begin() { begin(false); }
    void begin(bool apOverride);
    void handle();
    bool tryConnect(String ssid, String password);
    int savedNetworks() { return credMan.count(); };
    String savedSsid(int i) { return credMan.getSsid(i); };
    String savedPassword(int i) { return credMan.getPassword(i); };
    String connectedSsid() { return WiFi.status() == WL_CONNECTED ? WiFi.SSID(): ""; };
    WiFiScanInfo *scan(String ssid="");
    bool deleteSsid(String ssid);

private:
    bool began;
    bool apOverride;

    bool shouldReconnect();
    void enableAP(bool enable);
    WiFiScanInfo *consolidateScanInfo(String ssid="");
    String lastSsid;

    unsigned long lastConnectAttempt;

    WiFiCredentialManager credMan;
};

extern WiFiConnectionManager WCM;

#endif