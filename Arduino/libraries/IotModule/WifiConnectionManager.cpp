#include <ESP8266WiFi.h>
#include <limits.h>

#include <WiFiCredentialManager.h>
#include <WiFiConnectionManager.h>
#include <IotLogger.h>

WiFiConnectionManager::WiFiConnectionManager()
:began(false),
lastSsid("")
{
}

void WiFiConnectionManager::begin(bool apOverride) {
    if (!began) {
        credMan.load();
        this->apOverride = apOverride;
        lastConnectAttempt = ULONG_MAX-CONNECT_DELAY_MILLIS;
        if (apOverride || credMan.count() == 0) {
            enableAP(true);
        } else {
            WiFi.enableSTA(true);
            enableAP(false);
        }
    }

    began = true;
}

bool WiFiConnectionManager::shouldReconnect() {
    //TODO - Add logic to check if another bssid on our ssid has higher signal strength
    if (!apOverride) {
        if (credMan.count() > 0 && WiFi.status() != WL_CONNECTED && WiFi.softAPgetStationNum() == 0) {
            if (millis() - lastConnectAttempt > CONNECT_DELAY_MILLIS) {
                return true;
            }
        }
    }

    return false;
}

bool WiFiConnectionManager::tryConnect(String ssid, String password) {
    String curSsid = "";
    String curPassword;
    wl_status_t s;
    WiFiScanInfo *scanInfo = scan(ssid);

    if (!scanInfo) {
        return false;
    }

    if (WiFi.status() == WL_CONNECTED) {
        curSsid = WiFi.SSID();
        curPassword = WiFi.psk();
    }

    WiFi.begin(ssid, password, scanInfo->channel, scanInfo->bssid);
    delete scanInfo;

    Logger.infof("Trying connect to: %s", ssid.c_str());
    s = (wl_status_t)WiFi.waitForConnectResult();
    if (s != WL_CONNECTED) {
        Logger.warnf("Connect failed with status: %i", s);
        if (curSsid.length() > 0) {
            Logger.infof("Attempting reconnect to %s", curSsid.c_str());
            WiFi.begin(curSsid, curPassword);
            s = (wl_status_t)WiFi.waitForConnectResult();
            if (s != WL_CONNECTED) {
                Logger.warnf("Connect failed with status: %i", s);
            } else {
                Logger.info("Re-connection successful");
            }
        }
        return false;
    } else {
        Logger.info("Connection successful");
        lastSsid = ssid;
        credMan.add(ssid, password);
    }

    return true;
}

void WiFiConnectionManager::handle() {
    wl_status_t status = WL_DISCONNECTED;
    if (shouldReconnect()) {
        int8_t ret = WiFi.scanComplete();
        if (ret > 0) {
            Logger.info("Scan Complete");
            WiFiScanInfo *scanInfo = consolidateScanInfo();
            WiFiScanInfo *cur;

            for (int i = 0; i < credMan.count(); i++) {
                String ssid = credMan.getSsid(i);
                String password = credMan.getPassword(i);

                Logger.debugf("Searching scan results for: %s", ssid.c_str());
                cur = scanInfo;
                while (cur) {
                    if (ssid.compareTo(cur->ssid) == 0) {
                        if (password.length() > 0) {
                            if (cur->encType != ENC_TYPE_NONE) {
                                Logger.infof("Found, attempting connect with password: %s", password.c_str());
                                WiFi.begin(ssid, password, cur->channel, cur->bssid);
                            } else {
                                Logger.warn("Found, but encryption doesn't match");
                                continue;
                            }
                        } else {
                            if (cur->encType == ENC_TYPE_NONE) {
                                Logger.info("Found, attempting non-authenticated connect");
                                WiFi.begin(ssid, "", cur->channel, cur->bssid);
                            } else {
                                Logger.warn("Found, but encryption doesn't match");
                                continue;
                            }
                        }
                    }
                    cur = cur->next;
                }
                status = (wl_status_t)WiFi.waitForConnectResult();
                if (status == WL_CONNECTED) {
                    lastSsid = ssid;
                    Logger.infof("Connected, IP: %s", WiFi.localIP().toString().c_str());
                    break;
                } else {
                    Logger.warnf("Failed to connect with status: %i", status);
                }

            }

            if (status != WL_CONNECTED) {
                enableAP(true);
            }

            lastConnectAttempt = millis();
            
            WiFi.scanDelete();
            delete scanInfo;
        } else if (ret == 0) {
            Logger.warn("No networks found");
            enableAP(true);
        } else if (ret == WIFI_SCAN_FAILED) {  //FAILED means it hasn't been started
            Logger.info("Scan started");
            WiFi.scanNetworks(true);
        }
    } else {
        if (WiFi.status() == WL_CONNECTED && WiFi.softAPgetStationNum() == 0) {
            enableAP(false);
        }
    }
}

void WiFiConnectionManager::enableAP(bool enable) {
    if (enable) {
        uint8_t mac[6];
        uint32_t ip;
        char ssid[33];
        WiFi.macAddress(mac);
        snprintf(ssid, 33, CONFIG_SSID, mac[3], mac[4], mac[5]);
        ip = WiFi.softAPIP().v4();
        Logger.debugf("Current AP IP: %08X", ip);
        if ((WiFi.getMode() & WIFI_AP) == 0 || strcmp(ssid, WiFi.softAPSSID().c_str()) != 0
            || ip != CONFIG_AP_IP) {
            Logger.info("Configuring access point...");
            WiFi.enableSTA(false);
            WiFi.softAPConfig(CONFIG_AP_IP, CONFIG_AP_IP, CONFIG_AP_MASK);
            WiFi.softAP(ssid, CONFIG_PASSWORD);
        }
    } else {
        WiFi.enableAP(false);
    }
}

WiFiScanInfo *WiFiConnectionManager::scan(String ssid) {
    Logger.debug("WCM scan starting");
    WiFi.scanNetworks();
    
    WiFiScanInfo *ret = consolidateScanInfo(ssid);

    WiFi.scanDelete();

    return ret;
}

WiFiScanInfo *WiFiConnectionManager::consolidateScanInfo(String ssid) {
    int c = WiFi.scanComplete();
    uint8_t *bssid;
    WiFiScanInfo *head = NULL;
    WiFiScanInfo *cur;
    WiFiScanInfo *tmp = NULL;
    char bssidBuf[18];

    for (int i = 0; i < c; i++) {
        if (!tmp) {
            tmp = new WiFiScanInfo();
            tmp->next = NULL;
        }
        WiFi.getNetworkInfo(i, tmp->ssid, tmp->encType, tmp->rssi, bssid, tmp->channel, tmp->hidden);

        if (ssid.length() == 0 || ssid.compareTo(tmp->ssid) == 0) {
            if (!head) {
                head = tmp;
                memcpy(tmp->bssid, bssid, 6);
                tmp = NULL;
            } else {
                cur = head;
                while (1) {
                    if (cur->ssid.compareTo(tmp->ssid) == 0) {
                        if (tmp->rssi > cur->rssi) {
                            cur->rssi = tmp->rssi;
                            memcpy(cur->bssid, bssid, 6);
                            cur->channel = tmp->channel;
                        }
                        break;
                    }
                    if (!cur->next) {
                        cur->next = tmp;
                        memcpy(tmp->bssid, bssid, 6);
                        tmp = NULL;
                        break;
                    }
                    cur = cur->next;
                }
            }
        }
    }

    cur = head;
    while (cur) {
        sprintf(bssidBuf, "%02X:%02X:%02X:%02X:%02X:%02X", cur->bssid[0], cur->bssid[1], cur->bssid[2], cur->bssid[3], cur->bssid[4], cur->bssid[5]);
        cur->bssidStr = String(bssidBuf);
        cur = cur->next;
    }

    return head;
}

bool WiFiConnectionManager::deleteSsid(String ssid) {
    if (ssid.compareTo(WiFi.SSID()) == 0) {
        WiFi.disconnect(false);
    }

    if (ssid.compareTo(lastSsid) == 0) {
        lastSsid = "";
    }

    return credMan.deleteSsid(ssid);
}

WiFiConnectionManager WCM;