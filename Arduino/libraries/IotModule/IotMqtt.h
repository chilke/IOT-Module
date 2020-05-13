#ifndef IOT_MQTT_H
#define IOT_MQTT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define MQTT_CONNECT_RETRY 5000
#define MQTT_RESPONSE_TIMEOUT 10000

#define MQTT_CA_CERT_FILE "/ca.crt"
#define MQTT_CLIENT_CERT_FILE "/client.crt"
#define MQTT_CLIENT_KEY_FILE "/client.key"

#define MQTT_CA_CERT_BAK_FILE "/ca.crt.bak"
#define MQTT_CLIENT_CERT_BAK_FILE "/client.crt.bak"
#define MQTT_CLIENT_KEY_BAK_FILE "/client.key.bak"

class IotMqtt {
public:
    IotMqtt();
    void init();
    void handle();

    bool backupCerts();
    bool restoreCerts();
    void messageReceived(char* topic, char* payload, unsigned int length);
private:
    bool loadCerts();
    void connect(uint32_t m);
    void sendDeviceInfo();
    void sendDeviceState();
    void sendSchedules();

    BearSSL::X509List cert;
    BearSSL::X509List client_crt;
    BearSSL::PrivateKey key;
    WiFiClientSecure net;

    bool loaded;
    uint32_t lastConnectAttempt;

    PubSubClient client;
};

extern IotMqtt Mqtt;

#endif