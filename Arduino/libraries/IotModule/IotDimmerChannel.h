#ifndef DIMMER_CHANNEL_H
#define DIMMER_CHANNEL_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DIM_CH_NAME_ATTR "nm"
#define DIM_CH_MAX_ATTR "mx"
#define DIM_CH_MIN_ATTR "mn"
#define DIM_CH_ID_ATTR "id"
#define DIM_CH_SWITCH_ATTR "sw"

class IotDimmerChannel {
public:
    String name;
    uint16_t maxValue;
    uint16_t minValue;
    uint8_t id;
    bool isSwitch;

    void fromJson(JsonObject &obj);
    void toJson(JsonObject &obj);

    static int idFromJson(JsonObject &obj);
};

#endif