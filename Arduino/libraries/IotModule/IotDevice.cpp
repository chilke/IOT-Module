#include <Arduino.h>
#include <FS.h>

#include <IotModule.h>

#include <IotDevice.h>

IotDevice::IotDevice() {
    loaded = false;
    syncDevice = false;
    syncState = false;
    stateUpdateTime = 0;
}

bool IotDevice::init() {
    if (loaded) {
        return true;
    }
    
    if (SPIFFS.exists("/device_config.json")) {
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        File f = SPIFFS.open("/device_config.json", "r");
        DeserializationError err = deserializeJson(doc, f);

        if (!err && doc.containsKey(DEVICE_CLIENT_ATTR)) {
            JsonObject obj = doc.as<JsonObject>();
            clientID = doc[DEVICE_CLIENT_ATTR].as<String>();
            fromJson(obj);
            loaded = true;
        }

        f.close();
    }

    return loaded;
}

void IotDevice::handle() {
    if (stateUpdateTime != 0) {
        uint32_t m = millis();
        if (m - stateUpdateTime > STATE_UPDATE_TIME) {
            Logger.debug("Persisting device state");
            syncState = true;
            stateUpdateTime = 0;
            persist();
        }
    }
}

bool IotDevice::setClientID(String cid) {
    if (SPIFFS.exists("/device_config.json")) {
        return false;
    }

    clientID = cid;
    loaded = true;
    return true;
}

void IotDevice::persist() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    File f = SPIFFS.open("/device_config.json", "w");
    JsonObject obj = doc.to<JsonObject>();
    toJson(obj);
    serializeJson(obj, f);
    f.close();
}

void IotDevice::fromJson(JsonObject &obj) {
    String tmpStr = obj[DEVICE_TYPE_ATTR].as<String>();
    if (tmpStr == DIMMER_TYPE_NAME) {
        type = DeviceTypeDimmer;
        JsonArray chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
        for (JsonObject ch : chs) {
            channels[IotDimmerChannel::idFromJson(ch)].fromJson(ch);
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

void IotDevice::toJson(JsonObject &obj) {
    obj[DEVICE_CLIENT_ATTR] = clientID;
    String typeStr = "";
    if (type == DeviceTypeDimmer) {
        typeStr = DIMMER_TYPE_NAME;

        JsonArray chs = obj.createNestedArray(DEVICE_CH_ATTR);
        for (int i = 0; i < DIMMER_CH_CNT; i++) {
            JsonObject ch = chs.createNestedObject();
            channels[i].toJson(ch);
        }
    }
    obj[DEVICE_TYPE_ATTR] = typeStr;
    obj[DEVICE_LOC_ATTR] = location;
    obj[DEVICE_TZ_ATTR] = timezone;
}

void IotDevice::updateState(JsonObject &obj) {
    if (type == DeviceTypeDimmer) {
        JsonArray chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
        bool updated = false;
        for (JsonObject ch : chs) {
            if (channels[IotDimmerChannel::idFromJson(ch)].updateState(ch)) {
                updated = true;
            }
        }

        if (updated) {
            stateUpdateTime = millis();
            if (stateUpdateTime == 0) {
                stateUpdateTime = 1;
            }
        }
    }
}

IotDevice Device;