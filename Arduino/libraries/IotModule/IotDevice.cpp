#include <Arduino.h>
#include <FS.h>

#include <IotModule.h>

#include <IotDevice.h>

IotDevice::IotDevice() {
    syncDevice = false;
    syncState = false;
    stateUpdateTime = 0;
}

void IotDevice::init() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    if (SPIFFS.exists("/device_info.json")) {
        File f = SPIFFS.open("/device_info.json", "r");
        DeserializationError err = deserializeJson(doc, f);

        if (!err) {
            JsonObject obj = doc.as<JsonObject>();
            updateInfo(obj);
        }

        f.close();
        doc.clear();
    }
    if (SPIFFS.exists("/device_state.json")) {
        File f = SPIFFS.open("/device_state.json", "r");
        DeserializationError err = deserializeJson(doc, f);

        if (!err) {
            JsonObject obj = doc.as<JsonObject>();
            updateState(obj, false);
        }

        f.close();
    }
}

void IotDevice::handle() {
    if (stateUpdateTime != 0) {
        uint32_t m = millis();
        if (m - stateUpdateTime > STATE_UPDATE_TIME) {
            Logger.debug("Persisting device state");
            syncState = true;
            stateUpdateTime = 0;
            persistState();
        }
    }
}

void IotDevice::persistInfo() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    File f = SPIFFS.open("/device_info.json", "w");
    JsonObject obj = doc.to<JsonObject>();
    infoJson(obj);
    serializeJson(obj, f);
    f.close();
}

void IotDevice::persistState() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    File f = SPIFFS.open("/device_state.json", "w");
    JsonObject obj = doc.to<JsonObject>();
    stateJson(obj);
    serializeJson(obj, f);
    f.close();
}

void IotDevice::updateInfo(JsonObject &obj) {
    clientID = obj[DEVICE_CLIENT_ATTR] | String();
    mqttHost = obj[DEVICE_HOST_ATTR] | String();
    mqttPort = obj[DEVICE_PORT_ATTR];
    String tmpStr = obj[DEVICE_TYPE_ATTR].as<String>();
    if (tmpStr == DIMMER_TYPE_NAME) {
        type = DeviceTypeDimmer;
        JsonArray chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
        for (JsonObject ch : chs) {
            channels[IotDimmerChannel::idFromJson(ch)].updateInfo(ch);
        }
    } else {
        type = DeviceTypeNone;
    }
    location = obj[DEVICE_LOC_ATTR] | String();
    tmpStr = obj[DEVICE_TZ_ATTR] | String();
    if (tmpStr != timezone) {
        timezone = tmpStr;
        Time.setTz(timezone);
    }
}

void IotDevice::infoJson(JsonObject &obj) {
    obj[DEVICE_CLIENT_ATTR] = clientID;
    obj[DEVICE_HOST_ATTR] = mqttHost;
    obj[DEVICE_PORT_ATTR] = mqttPort;
    String typeStr = "";
    if (type == DeviceTypeDimmer) {
        typeStr = DIMMER_TYPE_NAME;

        JsonArray chs;

        if (obj.containsKey(DEVICE_CH_ATTR)) {
            chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
            for (int i = 0; i < DIMMER_CH_CNT; i++) {
                JsonObject ch = chs[i];
                channels[i].infoJson(ch);
            }
        } else {
            chs = obj.createNestedArray(DEVICE_CH_ATTR);
            for (int i = 0; i < DIMMER_CH_CNT; i++) {
                JsonObject ch = chs.createNestedObject();
                channels[i].infoJson(ch);
            }
        }
    }
    obj[DEVICE_TYPE_ATTR] = typeStr;
    obj[DEVICE_LOC_ATTR] = location;
    obj[DEVICE_TZ_ATTR] = timezone;
}

void IotDevice::updateState(JsonObject &obj) {
    updateState(obj, true);
}

void IotDevice::updateState(JsonObject &obj, bool sync) {
    if (type == DeviceTypeDimmer) {
        JsonArray chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
        for (JsonObject ch : chs) {
            channels[IotDimmerChannel::idFromJson(ch)].updateState(ch);
        }

        if (sync) {
            stateUpdateTime = millis();
            if (stateUpdateTime == 0) {
                stateUpdateTime = 1;
            }
        }
    }
}

void IotDevice::stateJson(JsonObject &obj) {
    if (type == DeviceTypeDimmer) {
        JsonArray chs;
        if (obj.containsKey(DEVICE_CH_ATTR)) {
            chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
            for (int i = 0; i < DIMMER_CH_CNT; i++) {
                JsonObject ch = chs[i];
                channels[i].stateJson(ch);
            }
        } else {
            chs = obj.createNestedArray(DEVICE_CH_ATTR);
            for (int i = 0; i < DIMMER_CH_CNT; i++) {
                JsonObject ch = chs.createNestedObject();
                channels[i].stateJson(ch);
            }
        }
    }
}

bool IotDevice::stateFromJson(DeviceState &state, JsonObject &obj) {
    if (type == DeviceTypeDimmer) {

    } else {
        return false;
    }
    
    return true;
}

bool IotDevice::stateToJson(DeviceState &state, JsonObject &obj) {
    if (type == DeviceTypeDimmer) {

    } else {
        return false;
    }

    return true;
}

IotDevice Device;