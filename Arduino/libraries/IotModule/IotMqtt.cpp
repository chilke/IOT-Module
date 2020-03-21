#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotMqtt.h>

void messageReceived(char* topic, byte* payload, unsigned int length) {
    Logger.infof("Recieved [%s]: %.*s", topic, length, (char*)payload);
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError err = deserializeJson(doc, (char *)payload, length);

    if (!err) {

    } else {
        Logger.error("MQTT json error");
    }
}

bool IotMqtt::publishMessage(const char *message) {
    if (!client.connected()) {
        return false;
    }
    sprintf(topicBuf, "t/%s/in", Device.clientID.c_str());
    if (client.publish(topicBuf, message)) {
        return true;
    }

    return false;
}

bool IotMqtt::sendDeviceInfo() {
    String message;
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj = doc.to<JsonObject>();
    Device.toJson(obj);
    serializeJson(obj, message);
    return publishMessage(message.c_str());
}

void IotMqtt::init() {
    loaded = false;
    if (Device.init()) {
        lastConnectAttempt = millis() - MQTT_CONNECT_RETRY;

        cert.append(cacert);
        client_crt.append(client_cert);
        key.parse(privkey);
        hostname = "a1r32q860r2zlh-ats.iot.us-east-2.amazonaws.com";
        port = 8883;

        net.setTrustAnchors(&cert);
        net.setClientRSACert(&client_crt, &key);

        client.setServer(hostname.c_str(), port);
        client.setClient(net);
        client.setCallback(messageReceived);

        loaded = true;
    } else {
        Logger.debug("Device init failed, skipping MQTT init");
    }
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
            }
        }
        lastConnectAttempt = m;
    }
}

void IotMqtt::handle() {
    if (loaded) {
        uint32_t m = millis();
        if (client.connected()) {
            client.loop();

            if (responseTimer != 0 && m > responseTimer) {
                Logger.warn("MQTT response timed out, disconnecting");
                client.disconnect();
            } else if (sendResponse) {
                if (!publishMessage("{}")) {
                    Logger.error("sendResponse failed, disconnecting");
                    client.disconnect();
                }
            }
        } else {
            connect(m);
        }
    } else {
        init();
    }
}

IotMqtt Mqtt;