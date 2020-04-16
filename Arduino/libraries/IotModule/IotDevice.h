#ifndef DEVICE_H
#define DEVICE_H

#include <time.h>
#include <ArduinoJson.h>
#include <IotDimmer.h>

#define DEVICE_CLIENT_ATTR "ci"
#define DEVICE_HOST_ATTR "hn"
#define DEVICE_PORT_ATTR "pt"
#define DEVICE_TYPE_ATTR "tp"
#define DEVICE_LOC_ATTR "lo"
#define DEVICE_TZ_ATTR "tz"
#define DEVICE_PERIPHERAL_ATTR "pr"
#define DEVICE_LAST_UPDATE_ATTR "ud"

#define STATE_UPDATE_TIME 1000

typedef enum device_type_t {
    DeviceTypeDimmer,
    DeviceTypeNone
} DeviceType;

typedef union device_state_t {
    DimmerState dimmerState;
} DeviceState;

typedef union device_slave_t {
    IotDimmer dimmer;
} DeviceSlave;

class IotDevice {
public:
    String clientID;
    String mqttHost;
    uint16_t mqttPort;
    DeviceType type;
    String location;
    String timezone;

    uint32_t lastStateUpdate;

    DeviceSlave slave;

    uint32_t stateUpdateTime;
    bool syncPic;

    bool syncDevice;
    bool syncState;

    IotDevice();
    void init();
    void persistInfo();
    void persistState();
    void updateInfo(JsonObject &obj);
    void updateState(DeviceState &state);
    void updateState(JsonObject &obj);
    void updateState(JsonObject &obj, bool sync);
    void infoJson(JsonObject &obj);
    void stateJson(JsonObject &obj);
    void handle();

    bool stateFromJson(DeviceState &state, JsonObject &obj);
    bool stateToJson(DeviceState &state, JsonObject &obj);
};

extern IotDevice Device;

#endif