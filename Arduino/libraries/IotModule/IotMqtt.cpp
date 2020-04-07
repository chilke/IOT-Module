#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotMqtt.h>

void mqttMessageReceived(char* topic, byte* payload, unsigned int length) {
    Mqtt.messageReceived(topic, (char*)payload, length);
}

void IotMqtt::messageReceived(char* topic, char* payload, unsigned int length) {
    Logger.infof("Received [%s]: %.*s", topic, length, payload);
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError err = deserializeJson(doc, payload, length);

    if (err) {
        Logger.error("MQTT json error");
        return;
    }

    if (doc.containsKey("cmd")) {
        String cmd = doc["cmd"];

        JsonObject obj = doc.as<JsonObject>();

        if (cmd == "update_state") {
            Logger.debug("Update device state");
            Device.updateState(obj);
        } else if (cmd == "update_device") {
            Logger.debug("Updated device info");
            Device.fromJson(obj);
            Device.syncDevice = true;
        } else if (cmd == "get_devices") {
            queryReceived = true;
        }
    }
}

void IotMqtt::sendDeviceInfo(const char topic[], const char cmd[], bool aws) {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj = doc.to<JsonObject>();
    Device.toJson(obj);
    String buffer;
    obj["cmd"] = cmd;
    obj["lambda"] = aws;
    serializeJson(obj, buffer);

    client.publish(topic, buffer.c_str());
}

void IotMqtt::init() {
    loaded = false;
    if (Device.init()) {
        lastConnectAttempt = millis() - MQTT_CONNECT_RETRY;

        if (!loadCerts()) {
            return;
        }
        
        hostname = "a1r32q860r2zlh-ats.iot.us-east-2.amazonaws.com";
        port = 8883;

        net.setTrustAnchors(&cert);
        net.setClientRSACert(&client_crt, &key);

        client.setServer(hostname.c_str(), port);
        client.setClient(net);
        client.setCallback(mqttMessageReceived);

        loaded = true;
    }
}

void IotMqtt::connect(uint32_t m) {
    if (m - lastConnectAttempt > MQTT_CONNECT_RETRY) {
        Logger.debug("MQTT connecting ");
        if (client.connect(Device.clientID.c_str()))
        {
            char topicBuf[20];
            Logger.debug("connected!");
            sprintf(topicBuf, "to/%s", Device.clientID.c_str());
            if (!client.subscribe(topicBuf)) {
                Logger.debugf("Subscribe failed to %s", topicBuf);
                client.disconnect();
            } else if (!client.subscribe("to/things")) {
                Logger.debug("Subscribe failed to all");
                client.disconnect();
            } else {
                Device.syncDevice = true;
                queryReceived = false;
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

            if (Device.syncDevice) {
                Logger.debug("Sending device info");
                sendDeviceInfo("to/apps", "update_device", true);
                Device.syncDevice = false;
                Device.syncState = false;
            } else if (Device.syncState) {
                Logger.debug("Sending state info");
                sendDeviceInfo("to/apps", "update_state", false);
                Device.syncState = false;
            } else if (queryReceived) {
                sendDeviceInfo("to/apps", "get_devices_resp", false);
                queryReceived = false;
            }
        } else {
            connect(m);
        }
    } else {
        init();
    }
}

bool IotMqtt::loadCerts() {
    if (!SPIFFS.exists(MQTT_CA_CERT_FILE)) {
        Logger.error("cacert file doesn't exist");
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_CERT_FILE)) {
        Logger.error("client cert file doesn't exist");
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_KEY_FILE)) {
        Logger.error("client key file doesn't exist");
        return false;
    }

    bool success = true;
    File caFile = SPIFFS.open(MQTT_CA_CERT_FILE, "r");
    File crtFile = SPIFFS.open(MQTT_CLIENT_CERT_FILE, "r");
    File keyFile = SPIFFS.open(MQTT_CLIENT_KEY_FILE, "r");

    size_t maxSize = caFile.size();

    if (crtFile.size() > maxSize) {
        maxSize = crtFile.size();
    }

    if (keyFile.size() > maxSize) {
        maxSize = keyFile.size();
    }

    uint8_t *buffer = new uint8_t[maxSize];

    size_t len = caFile.read(buffer, maxSize);
    if (cert.getCount() == 0 && !cert.append(buffer, len)) {
        Logger.error("Failed to load cacert");
        success = false;
    } else {
        len = crtFile.read(buffer, maxSize);
        if (client_crt.getCount() == 0 && !client_crt.append(buffer, len)) {
            Logger.error("Failed to load client cert");
            success = false;
        } else {
            len = keyFile.read(buffer, maxSize);
            if (!key.parse(buffer, len)) {
                Logger.error("Failed to load client key");
                success = false;
            }
        }
    }

    delete[] buffer;
    caFile.close();
    crtFile.close();
    keyFile.close();

    return success;
}

bool IotMqtt::backupCerts() {
    if (!SPIFFS.exists(MQTT_CA_CERT_FILE)) {
        Logger.errorf("Backup certs %s does not exist", MQTT_CA_CERT_FILE);
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_CERT_FILE)) {
        Logger.errorf("Backup certs %s does not exist", MQTT_CLIENT_CERT_FILE);
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_KEY_FILE)) {
        Logger.errorf("Backup certs %s does not exist", MQTT_CLIENT_KEY_FILE);
        return false;
    }

    if (SPIFFS.exists(MQTT_CA_CERT_BAK_FILE)) {
        if (!SPIFFS.remove(MQTT_CA_CERT_BAK_FILE)) {
            Logger.errorf("Backup certs failed to remove %s", MQTT_CA_CERT_BAK_FILE);
            return false;
        }
    }

    if (SPIFFS.exists(MQTT_CLIENT_CERT_BAK_FILE)) {
        if (!SPIFFS.remove(MQTT_CLIENT_CERT_BAK_FILE)) {
            Logger.errorf("Backup certs failed to remove %s", MQTT_CLIENT_CERT_BAK_FILE);
            return false;
        }
    }

    if (SPIFFS.exists(MQTT_CLIENT_KEY_BAK_FILE)) {
        if (!SPIFFS.remove(MQTT_CLIENT_KEY_BAK_FILE)) {
            Logger.errorf("Backup certs failed to remove %s", MQTT_CLIENT_KEY_BAK_FILE);
            return false;
        }
    }

    if (!SPIFFS.rename(MQTT_CA_CERT_FILE, MQTT_CA_CERT_BAK_FILE)) {
        Logger.errorf("Backup certs failed to rename %s", MQTT_CA_CERT_FILE);
        return false;
    }

    if (!SPIFFS.rename(MQTT_CLIENT_CERT_FILE, MQTT_CLIENT_CERT_BAK_FILE)) {
        Logger.errorf("Backup certs failed to rename %s", MQTT_CLIENT_CERT_FILE);
        return false;
    }

    if (!SPIFFS.rename(MQTT_CLIENT_KEY_FILE, MQTT_CLIENT_KEY_BAK_FILE)) {
        Logger.errorf("Backup certs failed to rename %s", MQTT_CLIENT_KEY_FILE);
        return false;
    }

    return true;
}

bool IotMqtt::restoreCerts() {
    if (!SPIFFS.exists(MQTT_CA_CERT_BAK_FILE)) {
        Logger.errorf("Restore certs %s does not exist", MQTT_CA_CERT_BAK_FILE);
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_CERT_BAK_FILE)) {
        Logger.errorf("Restore certs %s does not exist", MQTT_CLIENT_CERT_BAK_FILE);
        return false;
    }

    if (!SPIFFS.exists(MQTT_CLIENT_KEY_BAK_FILE)) {
        Logger.errorf("Restore certs %s does not exist", MQTT_CLIENT_KEY_BAK_FILE);
        return false;
    }

    if (SPIFFS.exists(MQTT_CA_CERT_FILE)) {
        if (!SPIFFS.remove(MQTT_CA_CERT_FILE)) {
            Logger.errorf("Restore certs failed to remove %s", MQTT_CA_CERT_FILE);
            return false;
        }
    }

    if (SPIFFS.exists(MQTT_CLIENT_CERT_FILE)) {
        if (!SPIFFS.remove(MQTT_CLIENT_CERT_FILE)) {
            Logger.errorf("Restore certs failed to remove %s", MQTT_CLIENT_CERT_FILE);
            return false;
        }
    }

    if (SPIFFS.exists(MQTT_CLIENT_KEY_FILE)) {
        if (!SPIFFS.remove(MQTT_CLIENT_KEY_FILE)) {
            Logger.errorf("Restore certs failed to remove %s", MQTT_CLIENT_KEY_FILE);
            return false;
        }
    }

    if (!SPIFFS.rename(MQTT_CA_CERT_BAK_FILE, MQTT_CA_CERT_FILE)) {
        Logger.errorf("Restore certs failed to rename %s", MQTT_CA_CERT_FILE);
        return false;
    }

    if (!SPIFFS.rename(MQTT_CLIENT_CERT_BAK_FILE, MQTT_CLIENT_CERT_FILE)) {
        Logger.errorf("Restore certs failed to rename %s", MQTT_CLIENT_CERT_FILE);
        return false;
    }

    if (!SPIFFS.rename(MQTT_CLIENT_KEY_BAK_FILE, MQTT_CLIENT_KEY_FILE)) {
        Logger.errorf("Restore certs failed to rename %s", MQTT_CLIENT_KEY_FILE);
        return false;
    }

    return true;
}

IotMqtt Mqtt;