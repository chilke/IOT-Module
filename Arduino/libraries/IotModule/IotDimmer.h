#ifndef DIMMER_H
#define DIMMER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DIM_CH_NAME_ATTR "nm"
#define DIM_CH_MAX_ATTR "mx"
#define DIM_CH_MIN_ATTR "mn"
#define DIM_CH_CUR_ATTR "cv"
#define DIM_CH_ID_ATTR "id"

#define DIMMER_CH_CNT 2

#define DIMMER_CH_NAME_MAX_LENGTH 50

#define DIMMER_TYPE_NAME "Dim"

typedef struct dimmer_channel_state_t {
    uint16_t values[DIMMER_CH_CNT];
} DimmerChannelState;

typedef struct dimmer_channel_t {
    char name[DIMMER_CH_NAME_MAX_LENGTH+1]
    uint16_t maxValue;
    uint16_t minValue;
    uint16_t curValue;
    uint8_t id;
} DimmerChannel;

class IotDimmer {
public:

    DimmerChannel channels[DIMMER_CH_CNT];

    void updateInfo(JsonObject &obj);
    void updateState(JsonObject &obj);
    void infoJson(JsonObject &obj);
    void stateJson(JsonObject &obj);

    static void stateFromJson(DimmerChannelState &state, JsonObject &obj);
    static void stateToJson(DimmeryChannelState &state, JsonObject &obj);
};

#endif