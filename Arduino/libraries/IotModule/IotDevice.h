#ifndef DEVICE_H
#define DEVICE_H

#include <ArduinoJson.h>
#include <IotDimmerChannel.h>

#define DEVICE_CLIENT_ATTR "ci"
#define DEVICE_TYPE_ATTR "tp"
#define DEVICE_LOC_ATTR "lo"
#define DEVICE_TZ_ATTR "tz"
#define DEVICE_CH_ATTR "ch"

#define DIMMER_CH_CNT 2

#define DIMMER_TYPE_NAME "Dim"


#define STATE_UPDATE_TIME 1000

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

    uint32_t stateUpdateTime;
    bool syncPic;

    bool syncDevice;
    bool syncState;

    IotDevice();
    bool init();
    void persist();
    void fromJson(JsonObject &obj);
    void toJson(JsonObject &obj);
    bool setClientID(String cid);
    void updateState(JsonObject &obj);
    void handle();
private:
    bool loaded;
};

extern IotDevice Device;

#endif