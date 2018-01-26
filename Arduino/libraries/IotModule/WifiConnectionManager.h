#ifndef WIFI_CONNECTION_MANAGER_H
#define WIFI_CONNECTION_MANAGER_H

#define CONNECT_DELAY_MILLIS 30000

#define CONFIG_SSID "CHILKE_IOT"
#define CONFIG_PASSWORD "iot-password"

void wcmBegin();
void setupConfigAP();
void wcmHandle();

#endif