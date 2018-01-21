#include "ESP8266WiFi.h"
#include "ArduinoOTA.h"
#include "limits.h"

#define CONNECT_DELAY_MILLIS 30000

const char* ssid = "HILKE_C";
const char* password = "6369807439";
unsigned long lastConnectAttempt = ULONG_MAX-CONNECT_DELAY_MILLIS;

void setup() {
  // Start Serial
  Serial.begin(115200);
  WiFi.persistent(false);
  // Setup PINs
  pinMode(4, INPUT_PULLUP);

  // Check for safemode startup
  uint8_t count = 0;
  while(digitalRead(4) == 0) {
    if (++count > 50) {
        Serial.println();
        Serial.println("Configuring access point...");
        /* You can remove the password parameter if you want the AP to be open. */
        WiFi.softAP("MyESP", "6369807439");
      
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        setupOTA();
        Serial.println("OTA Setup Waiting for upload");
        delay(1000);
        while (1) {
          ArduinoOTA.handle();
          delay(1000);
        }
    }
    delay(100);
  } // end check for safemode startup

  // Setup Over the Air update handler
  setupOTA();
}

void loop() {
  if ((WiFi.getMode() & WIFI_AP) || WiFi.status() != WL_CONNECTED) {
    if (!(WiFi.getMode() & WIFI_AP)) {
      Serial.println("Not connected, starting AP");
      char tmp[18];
      sprintf(tmp, "CHILKE_IOT_%06X", ESP.getChipId());
      WiFi.softAP(tmp, "password");
      Serial.print("Status after AP: ");
      Serial.println(WiFi.status());
    }
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
            Serial.print("Searching scan results for: ");
            Serial.println(ssid);
            for (uint8_t j = 0; j < ret; j++) {
              if (strncmp(ssid, WiFi.SSID(j).c_str(), 32) == 0) {
                char *password = wifiCredentialPassword(i);
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
            }  // end scan results loop
            if (WiFi.status() == WL_CONNECTED) {
              break;
            }
          } // end wifi credentials loop
        }
        WiFi.scanDelete();
        lastConnectAttempt = millis();
      } else if (ret == WIFI_SCAN_FAILED) {
        Serial.println("Scan started");
        WiFi.scanNetworks(true);
      }
    }
  }

  // Handle Over the Air update events
  ArduinoOTA.handle();
}
