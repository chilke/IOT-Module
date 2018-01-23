#include <IPAddress.h>
#include <ESP8266mDNS.h>
#include <MD5Builder.h>
#include <Updater.h>
#include <StreamString.h>
#include "Utility.h"
#include "UDPListener.h"
#include "OTA.h"

String password;
String nonce;
String md5;
ota_state_t state;
int size;
uint8_t command;
uint16_t ota_port;
uint16_t ota_udp_port;
IPAddress ota_ip;

void otaBegin() {
  MDNS.begin("myesp8266");
  if (password.length()) {
    MDNS.enableArduino(MY_UDP_PORT, true);
  } else {
    MDNS.enableArduino(MY_UDP_PORT);
  }
  state = OTA_IDLE;
}

void otaHandle(WiFiUDP *udp) {
  if (state == OTA_RUNUPDATE) {
    Serial.println("otaHandle, state = OTA_RUNUPDATE");
    udp->beginPacket(ota_ip, ota_udp_port);
    if (!Update.begin(size, command)) {
      StreamString ss;
      Update.printError(ss);
      udp->write("ERR: ", 5);
      udp->write(ss.c_str(), ss.length());
      udp->endPacket();
      state = OTA_IDLE;
      return;
    }
    udp->write("OK", 2);
    udp->endPacket();
    delay(100);
  
    Update.setMD5(md5.c_str());
    WiFiUDP::stopAll();
    WiFiClient::stopAll();
  
    WiFiClient client;
    if (!client.connect(ota_ip, ota_port)) {
      udpBegin();
      state = OTA_IDLE;
    }
  
    uint32_t written, total = 0;
    while (!Update.isFinished() && client.connected()) {
      int waited = 1000;
      while (!client.available() && waited--)
        delay(1);
      if (!waited){
        udpBegin();
        state = OTA_IDLE;
      }
      written = Update.write(client);
      if (written > 0) {
        client.print(written, DEC);
        total += written;
      }
    }
  
    if (Update.end()) {
      client.print("OK");
      client.stop();
      //let serial/network finish tasks that might be given in _end_callback
      delay(100);
      ESP.restart();
    } else {
      udpBegin();
      Update.printError(client);
      state = OTA_IDLE;
    }
  }
}

bool otaHandleCmd(unsigned char *cmdBuffer, int len, WiFiUDP *udp) {
  int i = 0;
  uint8_t cmd = parseInt((char *)cmdBuffer, i);
  Serial.print("otaHandleCmd: ");
  Serial.println(cmd);
  if (cmd == U_FLASH || cmd == U_SPIFFS) {
    Serial.print("state: ");
    Serial.println(state);
    if (state == OTA_IDLE) {
      int i = 1;
      command = cmd;
      ota_ip = udp->remoteIP();
      ota_udp_port = udp->remotePort();
      Serial.print("index before ota_port: ");
      Serial.println(i);
      ota_port = parseInt((char *)cmdBuffer, i);
      Serial.print("index after ota_port: ");
      Serial.println(i);
      Serial.print("ota_port: ");
      Serial.println(ota_port);
      Serial.print("index before size: ");
      Serial.println(i);
      size = parseInt((char *)cmdBuffer, i);
      Serial.print("index after size: ");
      Serial.println(i);
      Serial.print("size: ");
      Serial.println(size);
      md5 = parseNonSpaceString((char *)cmdBuffer, i);
      Serial.print("md5: ");
      Serial.print(md5);
      Serial.print(" Length: ");
      Serial.println(md5.length());
      if (md5.length() == 32) {
        if (password.length()) {
          MD5Builder nonce_md5;
          nonce_md5.begin();
          nonce_md5.add(String(micros()));
          nonce_md5.calculate();
          nonce = nonce_md5.toString();
    
          char auth_req[38];
          sprintf(auth_req, "AUTH %s", nonce.c_str());
          udp->beginPacket(ota_ip, ota_udp_port);
          udp->write((const uint8_t *)auth_req, strlen(auth_req));
          udp->endPacket();
          state = OTA_WAITAUTH;
        }
        else {
          Serial.println("State change to RUNUPDATE");
          state = OTA_RUNUPDATE;
        }
      }
    } else {
      state = OTA_IDLE;
    }
  } else if (cmd == U_AUTH) {
    if (state == OTA_WAITAUTH) {
      String cnonce = parseNonSpaceString((char *)&cmdBuffer[i], i);
      String response = parseNonSpaceString((char *)&cmdBuffer[i], i);
      if (cnonce.length() == 32 && response.length() == 32) {
        String challenge = password + ":" + nonce + ":" + cnonce;
        MD5Builder challengemd5;
        challengemd5.begin();
        challengemd5.add(challenge);
        challengemd5.calculate();
        String result = challengemd5.toString();

        if (result.equalsConstantTime(response)) {
          state = OTA_RUNUPDATE;
        } else {
          udp->beginPacket(ota_ip, ota_udp_port);
          udp->write((uint8_t *)"Authentication Failed", 21);
          udp->endPacket();
          state = OTA_IDLE;
        }
      } else {
        state = OTA_IDLE;
      }
    } else {
      state = OTA_IDLE;
    }
  } else {
    return false;
  }
  return true;
}
