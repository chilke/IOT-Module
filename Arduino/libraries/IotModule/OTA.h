#ifndef OTA_H
#define OTA_H

#include <WiFiUDP.h>

typedef enum {
  OTA_IDLE,
  OTA_WAITAUTH,
  OTA_RUNUPDATE
} ota_state_t;

typedef enum {
  OTA_AUTH_ERROR,
  OTA_BEGIN_ERROR,
  OTA_CONNECT_ERROR,
  OTA_RECEIVE_ERROR,
  OTA_END_ERROR
} ota_error_t;

bool otaHandleCmd(unsigned char *cmdBuffer, int len, WiFiUDP *udp);
void otaHandle(WiFiUDP *udp);
void otaBegin();

#endif