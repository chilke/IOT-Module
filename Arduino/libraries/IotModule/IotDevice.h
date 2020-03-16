#ifndef DEVICE_H
#define DEVICE_H

#include <ArduinoJson.h>

#define DIMMER_TYPE_NAME "DimmerSwitch"
#define SWITCH_TYPE_NAME "Switch"

typedef enum device_type_t {
    DeviceTypeDimmer,
    DeviceTypeSwitch,
    DeviceTypeNone
} DeviceType;

class IotDevice {
public:
    String clientID;
    DeviceType type;
    String location;
    String name;

    void fromJson(JsonObject &obj);
    void toJson(JsonObject &obj);
    void debug();
};

extern IotDevice Device;

#endif