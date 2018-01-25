#include <Arduino.h>
#include <WiFiUDP.h>
#include "OTA.h"
#include "UIClient.h"
#include "UDPListener.h"

#define CMD_BUF_LEN 100

WiFiUDP udp;
unsigned char cmdBuffer[CMD_BUF_LEN];

void udpBegin() {
  if (!udp.begin(MY_UDP_PORT)) {
    Serial.println("udpBegin failed to begin");
  }
  otaBegin();
}

void udpHandle() {
  int len = udp.parsePacket();
  if (len > 0) {
    Serial.println("Received udp packet");
    if (len >= CMD_BUF_LEN) {
      Serial.print("udp received packet too big: ");
      Serial.println(len);
    } else {
      udp.read(cmdBuffer, len);
      Serial.printf("%.*s", len, cmdBuffer);
      Serial.println();
      if (otaHandleCmd(cmdBuffer, len, &udp)) {
        
      } else if (uiClientHandleCmd(cmdBuffer, len)) {
        
      } else {
        Serial.print("udp received unknown command: ");
        Serial.println(cmdBuffer[0]);
      }
    }
  }
  otaHandle(&udp);
}

