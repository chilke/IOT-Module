#ifndef DEVICE_H
#define DEVICE_H

#include <ArduinoJson.h>
#include <IotDimmerChannel.h>

#define DEVICE_SYNC_ATTR "ns"
#define DEVICE_CLIENT_ATTR "ci"
#define DEVICE_TYPE_ATTR "tp"
#define DEVICE_LOC_ATTR "lo"
#define DEVICE_TZ_ATTR "tz"
#define DEVICE_CH_ATTR "ch"

#define DIMMER_CH_CNT 2

#define DIMMER_TYPE_NAME "Dim"

typedef enum device_type_t {
    DeviceTypeDimmer,
    DeviceTypeNone
} DeviceType;

class IotDevice {
public:
    String clientID;
    DeviceType type;
    String location;
    String timezone;

    IotDimmerChannel channels[DIMMER_CH_CNT];

    IotDevice();
    bool init();
    void persist();
    bool fromJson(JsonObject &obj);
    void toJson(JsonObject &obj);
    bool setClientID(String cid);

private:
    bool loaded;
};

extern IotDevice Device;

#endif