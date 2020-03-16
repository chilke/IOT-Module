#include <Arduino.h>

#include <IotLogger.h>
#include <IotDevice.h>

void IotDevice::init() {
    clientID = "CSH-12345678";
}

void IotDevice::fromJson(JsonObject &obj) {
    clientID = obj["ClientID"].as<String>();
    String typeStr = obj["Type"];
    if (typeStr == DIMMER_TYPE_NAME) {
        type = DeviceTypeDimmer;
    } else if (typeStr == SWITCH_TYPE_NAME) {
        type = DeviceTypeSwitch;
    } else {
        type = DeviceTypeNone;
    }
    name = obj["Name"].as<String>();
    location = obj["Location"].as<String>();
}

void IotDevice::toJson(JsonObject&obj) {
    obj["ClientID"] = clientID;
    String typeStr = "";
    if (type == DeviceTypeDimmer) {
        typeStr = DIMMER_TYPE_NAME;
    } else if (type == DeviceTypeSwitch) {
        typeStr = SWITCH_TYPE_NAME;
    }
    obj["Type"] = typeStr;
    obj["Name"] = name;
    obj["Location"] = location;
}

void IotDevice::debug() {
    Logger.debugf("clientID: %s", clientID.c_str());
    Logger.debugf("type: %d", type);
    Logger.debugf("name: %s", name.c_str());
    Logger.debugf("location: %s", location.c_str());
}

IotDevice Device;