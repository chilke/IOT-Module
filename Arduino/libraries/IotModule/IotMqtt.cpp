#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotMqtt.h>

void mqttMessageReceived(char* topic, byte* payload, unsigned int length) {
    Mqtt.messageReceived(topic, (char*)payload, length);
}

IotMqtt::IotMqtt() {
    loaded = false;
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
            JsonObject state = obj["st"];
            Device.updateState(state);
        } else if (cmd == "update_device") {
            Logger.debug("Updated device info");
            JsonObject device = obj["dv"];
            Device.updateInfo(device);
            Device.syncDevice = true;
        } else if (cmd == "get_devices") {
            Logger.debug("Received query");
            Device.syncDevice = true;
        } else if (cmd == "get_schedules") {
            Logger.debug("Get schedules");
            Scheduler.needsSync = true;
        } else if (cmd == "add_schedule") {
            Logger.debug("Add schedule");
            JsonObject schedule = obj["sc"];
            Scheduler.addSchedule(schedule);
            Scheduler.needsSync = true;
        } else if (cmd == "del_schedule") {
            Logger.debug("Delete schedule");
            if (obj.containsKey("id")) {
                int id = obj["id"];
                Scheduler.deleteSchedule(id);
            }
            Scheduler.needsSync = true;
        }
    }
}

void IotMqtt::sendDeviceInfo() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj = doc.createNestedObject("dv");
    Device.infoJson(obj);
    String buffer;
    doc["cmd"] = "device_info";
    serializeJson(doc, buffer);

    client.publish("to/apps", buffer.c_str());
}

void IotMqtt::sendDeviceState() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj = doc.createNestedObject("st");
    Device.stateJson(obj);
    String buffer;
    doc["cmd"] = "device_state";
    serializeJson(doc, buffer);

    client.publish("to/apps", buffer.c_str());
}

void IotMqtt::sendSchedules() {
    //Very hacky, but don't have enough memory to hold json schedules
    String buffer = "{\"cmd\":\"schedules\",\"ci\":\"";
    buffer += Device.clientID;
    buffer += "\",\"sc\":[";
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj;
    bool first = true;
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        obj = doc.to<JsonObject>();
        if (Scheduler.getSchedule(i, obj)) {
            if (!first) {
                buffer += ",";
            } else {
                first = false;
            }
            serializeJson(obj, buffer);
        }
        doc.clear();
    }
    buffer += "]}";

    client.publish("to/apps", buffer.c_str());
}

void IotMqtt::init() {
    if (Device.mqttHost.length() == 0) {
        return;
    }

    if (Device.mqttPort == 0) {
        return;
    }

    if (Device.clientID.length() == 0) {
        return;
    }

    if (!loadCerts()) {
        return;
    }
    
//        hostname = "a1r32q860r2zlh-ats.iot.us-east-2.amazonaws.com";
//    port = 8883;

    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    client.setServer(Device.mqttHost.c_str(), Device.mqttPort);
    client.setClient(net);
    client.setCallback(mqttMessageReceived);

    lastConnectAttempt = millis() - MQTT_CONNECT_RETRY;
    loaded = true;
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
                sendDeviceInfo();
                Device.syncDevice = false;
                Device.syncState = false;
            } else if (Device.syncState) {
                Logger.debug("Sending state info");
                sendDeviceState();
                Device.syncState = false;
            } else if (Scheduler.needsSync) {
                Logger.debug("Sending schedules");
                sendSchedules();
                Scheduler.needsSync = false;
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