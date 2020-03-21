#include <Arduino.h>
#include <FS.h>

#include <IotModule.h>

#include <IotDevice.h>

IotDevice::IotDevice() {
    loaded = false;
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
            loaded = fromJson(obj);
        }

        f.close();
    }

    return loaded;
}

bool IotDevice::setClientID(String cid) {
    if (SPIFFS.exists("/device_config.json")) {
        return false;
    }

    clientID = cid;
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

bool IotDevice::fromJson(JsonObject &obj) {
    String tmpStr = obj[DEVICE_TYPE_ATTR].as<String>();
    if (tmpStr == DIMMER_TYPE_NAME) {
        type = DeviceTypeDimmer;
        JsonArray chs = obj[DEVICE_CH_ATTR].as<JsonArray>();
        int i = 0;
        for (JsonObject ch : chs) {
            channels[i].fromJson(ch);
            if (channels[i].id != i) {
                Logger.errorf("Channel id mismatch, i:%i, id:%i", i, channels[i].id);
                return false;
            }
            i++;
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

    return true;
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

IotDevice Device;