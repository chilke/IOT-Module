#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotMqtt.h>

void messageReceived(char* topic, byte* payload, unsigned int length) {
    Logger.infof("Recieved [%s]: %.*s", topic, length, (char*)payload);
}

uint8_t IotMqtt::publishMessage(const char *message) {
    if (!client.connected()) {
        return MQTT_NOT_CONNECTED;
    }
    sprintf(topicBuf, "t/%s/in", Device.clientID.c_str());
    if (client.publish(topicBuf, message)) {
        return MQTT_SUCCESS;
    }

    return MQTT_FAILURE;
}

void IotMqtt::begin() {
    lastConnectAttempt = millis() - MQTT_CONNECT_RETRY;

    cert.append(cacert);
    client_crt.append(client_cert);
    key.parse(privkey);
    hostname = "a1r32q860r2zlh-ats.iot.us-east-2.amazonaws.com";
    port = 8883;

    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

//Set keepAlive=45 to avoid aws charges
    client.setServer(hostname.c_str(), port);
    client.setClient(net);
    client.setCallback(messageReceived);
}

void IotMqtt::connect(uint32_t m) {
    if (m - lastConnectAttempt > MQTT_CONNECT_RETRY) {
        Logger.debug("MQTT connecting ");
        if (client.connect(Device.clientID.c_str()))
        {
            Logger.debug("connected!");
            sprintf(topicBuf, "t/%s/out", Device.clientID.c_str());
            if (!client.subscribe(topicBuf)) {
                Logger.debugf("Subscribe failed to %s", topicBuf);
                client.disconnect();
            } else if (!client.subscribe("t/all/out")) {
                Logger.debug("Subscribe failed to all");
                client.disconnect();
            } else {
                responseTimer = 0;
                sendResponse = false;
                heartbeatTimer = m;
            }
        }
        lastConnectAttempt = m;
    }
}

void IotMqtt::handle() {
    uint32_t m = millis();
    if (client.connected()) {
        client.loop();

        if (responseTimer != 0 && responseTimer - m > MQTT_RESPONSE_TIMEOUT) {
            Logger.debug("MQTT response timed out, disconnecting");
            client.disconnect();
        } else if (sendResponse) {

        }
    } else {
        connect(m);
    }
}

IotMqtt Mqtt;