#ifndef DIMMER_CHANNEL_H
#define DIMMER_CHANNEL_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DIM_CH_NAME_ATTR "nm"
#define DIM_CH_MAX_ATTR "mx"
#define DIM_CH_MIN_ATTR "mn"
#define DIM_CH_CUR_ATTR "cv"
#define DIM_CH_ID_ATTR "id"

class IotDimmerChannel {
public:
    String name;
    uint16_t maxValue;
    uint16_t minValue;
    uint16_t curValue;
    uint8_t id;

    void updateInfo(JsonObject &obj);
    void updateState(JsonObject &obj);
    void infoJson(JsonObject &obj);
    void stateJson(JsonObject &obj);

    static int idFromJson(JsonObject &obj);
};

#endif