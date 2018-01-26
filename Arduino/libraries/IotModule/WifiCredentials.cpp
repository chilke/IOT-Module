#include <Arduino.h>
#include <EEPROM.h>

//  char *ssid1 = "HILKE1";
//  char *password1 = "1369807439";
//  char *ssid2 = "HILKE2";
//  char *password2 = "2369807439";
//  char *ssid3 = "HILKE3";
//  char *password3 = "3369807439";
//  char *ssid4 = "HILKE4";
//  char *password4 = "4369807439";
//  char *ssid5 = "HILKE5";
//  char *password5 = "5369807439";
//
//  saveWifiCredential(ssid5, password5);
//  saveWifiCredential(ssid4, password4);
//  saveWifiCredential(ssid1, password2);
//  saveWifiCredential(ssid3, password3);
//  saveWifiCredential(ssid2, password2);
//  saveWifiCredential(ssid1, password1);

#define MAX_CREDENTIAL_COUNT 5
#define SSID_SIZE 33
#define PASSWORD_SIZE 65

struct wifi_credential {
  char ssid[SSID_SIZE];
  char password[PASSWORD_SIZE];
};

struct wifi_credentials {
  uint8_t count;
  wifi_credential credentials[MAX_CREDENTIAL_COUNT];
};

#define WIFI_CREDENTIAL_SIZE sizeof(wifi_credential)
#define WIFI_CREDENTIALS_SIZE sizeof(wifi_credentials)

#define WIFI_CREDENTIALS_ADDR 0

wifi_credentials *_wc = 0;

void deleteWifiCredentials() {
  EEPROM.begin(1);
  EEPROM.write(0, 0);
  EEPROM.end();
}

void debugWifiCredentials() {
//  Serial.print("WIFI_CREDENTIAL_SIZE: ");
//  Serial.println(WIFI_CREDENTIAL_SIZE);
//  Serial.print("WIFI_CREDENTIALS_SIZE: ");
//  Serial.println(WIFI_CREDENTIALS_SIZE);
//  Serial.print("WIFI_CREDENTIAL_END_ADDRESS: ");
//  Serial.println(WIFI_CREDENTIAL_END_ADDRESS);

  Serial.print(_wc->count);
  Serial.println(" wifi credentials:");
  for (uint8_t i = 0; i < _wc->count; i++) {

    Serial.printf("%s %s\r\n", _wc->credentials[i].ssid, _wc->credentials[i].password);
  }
}

void clearWifiCredentials() {
  if (_wc) {
    delete _wc;

    _wc = 0;
  }
}

char *wifiCredentialSSID(uint8_t i) {
  return _wc->credentials[i].ssid;
}

char *wifiCredentialPassword(uint8_t i) {
  return _wc->credentials[i].password;
}

void loadWifiCredentials() {
  clearWifiCredentials();
  EEPROM.begin(WIFI_CREDENTIALS_SIZE);

  _wc = new wifi_credentials();

  EEPROM.get(WIFI_CREDENTIALS_ADDR, *_wc);
}

uint8_t wifiCredentialCount() {
  if (_wc) {
    return _wc->count;
  }
  return 0;
}

void saveWifiCredential(char *ssid, char *password) {
  loadWifiCredentials();

  int i;
  
  for (i = 0; i < _wc->count; i++) {
    if (strncmp(ssid, _wc->credentials[i].ssid, SSID_SIZE) == 0) {
      --_wc->count;
      break;
    }
  }

  if (i > 0)
  {
    if (i == MAX_CREDENTIAL_COUNT) {
      --i;
    }

    for (i; i > 0; i--) {
      memcpy(_wc->credentials[i].ssid, _wc->credentials[i-1].ssid, SSID_SIZE);
      memcpy(_wc->credentials[i].password, _wc->credentials[i-1].password, PASSWORD_SIZE);
    }
  }

  memcpy(_wc->credentials[0].ssid, ssid, SSID_SIZE);
  memcpy(_wc->credentials[0].password, password, PASSWORD_SIZE);

  if (_wc->count < MAX_CREDENTIAL_COUNT) {
    ++_wc->count;
  }

  EEPROM.put(WIFI_CREDENTIALS_ADDR, *_wc);

  EEPROM.end();

  clearWifiCredentials();
}
