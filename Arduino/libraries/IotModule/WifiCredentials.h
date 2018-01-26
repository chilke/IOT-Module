
void deleteWifiCredentials();
void debugWifiCredentials();
void clearWifiCredentials();
char *wifiCredentialSSID(uint8_t i);
char *wifiCredentialPassword(uint8_t i);
void loadWifiCredentials();
uint8_t wifiCredentialCount();
void saveWifiCredential(char *ssid, char *password);