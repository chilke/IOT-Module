#include <ESP8266WiFi.h>
#include <limits.h>

#include "WifiCredentials.h"
#include "WifiConnectionManager.h"

unsigned long lastConnectAttempt = ULONG_MAX-CONNECT_DELAY_MILLIS;

void wcmBegin() {
    WiFi.persistent(false);
    WiFi.enableSTA(true);
    WiFi.enableAP(false);
}

void setupConfigAP() {
    Serial.println();
    Serial.println("Configuring access point...");
/* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(CONFIG_SSID, CONFIG_PASSWORD);
}

void wcmHandle() {
    if (!(WiFi.getMode() & WIFI_AP) && WiFi.status() != WL_CONNECTED) {
        if (millis() - lastConnectAttempt > CONNECT_DELAY_MILLIS) {
            int8_t ret = WiFi.scanComplete();
            if (ret >= 0) {
                Serial.println("Scan Complete");
                if (ret == 0) {
                    Serial.println("No networks found");
                } else {
                    loadWifiCredentials();
                    for (uint8_t i = 0; i < wifiCredentialCount(); i++) {
                        char *ssid = wifiCredentialSSID(i);
                        char *password = wifiCredentialPassword(i);

                        Serial.print("Searching scan results for: ");
                        Serial.println(ssid);
                        for (uint8_t j = 0; j < ret; j++) {
                            if (strncmp(ssid, WiFi.SSID(j).c_str(), 32) == 0) {
                                if (strlen(password) > 0) {
                                    if (WiFi.encryptionType(j) != ENC_TYPE_NONE) {
                                        Serial.print("Found, attempting connect with password: ");
                                        Serial.println(password);
                                        WiFi.begin(ssid, password);
                                    } else {
                                        Serial.println("Found, but encryption doesn't match");
                                        continue;
                                    }
                                } else {
                                    if (WiFi.encryptionType(j) == ENC_TYPE_NONE) {
                                        Serial.println("Found, attempting non-authenticated connect");
                                        WiFi.begin(ssid);
                                    } else {
                                        Serial.println("Found, but encryption doesn't match");
                                        continue;
                                    }
                                }
                                wl_status_t s = WiFi.status();
                                while (s == WL_DISCONNECTED) {
                                    Serial.print(".");
                                    delay(500);
                                    s = WiFi.status();
                                }
                                if (s == WL_CONNECTED) {
                                    WiFi.softAPdisconnect(true);
                                    Serial.print("Connected, IP: ");
                                    Serial.println(WiFi.localIP());
                                    break;
                                } else {
                                    Serial.print("Connect failed: ");
                                    Serial.println(s);
                                }
                            }
                        }
                        if (WiFi.status() == WL_CONNECTED) {
                            break;
                        }
                    }
                }
                WiFi.scanDelete();

                lastConnectAttempt = millis();
            } else if (ret == WIFI_SCAN_FAILED) {
                Serial.println("Scan started");
                WiFi.scanNetworks(true);
            }

        }
    }
}