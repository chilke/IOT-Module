#ifndef DIMMER_H
#define DIMMER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DIM_CH_NAME_ATTR "nm"
#define DIM_CH_MAX_ATTR "mx"
#define DIM_CH_MIN_ATTR "mn"
#define DIM_CH_CUR_ATTR "cv"
#define DIM_CH_ID_ATTR "id"
#define DIM_CH_ATTR "ch"

#define DIM_CH_CNT 2

#define DIM_CH_NAME_LENGTH 50

#define DIM_TYPE_NAME "Dim"

typedef struct dimmer_state_t {
    uint16_t values[DIM_CH_CNT];
} DimmerState;

typedef struct dimmer_channel_t {
    char name[DIM_CH_NAME_LENGTH+1];
    uint16_t maxValue;
    uint16_t minValue;
    uint16_t curValue;
    uint8_t id;
} DimmerChannel;

class IotDimmer {
public:
    DimmerChannel channels[DIM_CH_CNT];

    void updateInfo(JsonObject &obj);
    void updateState(DimmerState &state);
    void updateState(JsonObject &obj);
    void infoJson(JsonObject &obj);
    void stateJson(JsonObject &obj);

    void stateFromJson(DimmerState &state, JsonObject &obj);
    void stateToJson(DimmerState &state, JsonObject &obj);
};

#endif