#include <IotModule.h>

void IotDimmer::updateInfo(JsonObject &obj) {
    JsonArray chs = obj[DIM_CH_ATTR];
    for (JsonObject ch : chs) {
        int id = ch[DIM_CH_ID_ATTR];
        channels[id].maxValue = ch[DIM_CH_MAX_ATTR].as<uint16_t>();
        channels[id].minValue = ch[DIM_CH_MIN_ATTR].as<uint16_t>();
        channels[id].id = id;
        const char *name = ch[DIM_CH_NAME_ATTR];
        if (name != NULL) {
            strncpy(channels[id].name, name, DIM_CH_NAME_LENGTH);
        } else {
            strncpy(channels[id].name, "N/A", DIM_CH_NAME_LENGTH);
        }
    }
}

void IotDimmer::infoJson(JsonObject &obj) {
    JsonArray chs;

    if (obj.containsKey(DIM_CH_ATTR)) {
        chs = obj[DIM_CH_ATTR];
    } else {
        chs = obj.createNestedArray(DIM_CH_ATTR);
        for (int i = 0; i < DIM_CH_CNT; i++) {
            chs.createNestedObject();
        }
    }

    int i = 0;
    for (JsonObject ch : chs) {
        if (!ch.containsKey(DIM_CH_ID_ATTR)) {
            ch[DIM_CH_ID_ATTR] = i;
        } else {
            i = ch[DIM_CH_ID_ATTR];
        }
        ch[DIM_CH_NAME_ATTR] = channels[i].name;
        ch[DIM_CH_MAX_ATTR] = channels[i].maxValue;
        ch[DIM_CH_MIN_ATTR] = channels[i].minValue;

        i++;
    }
}

void IotDimmer::updateState(JsonObject &obj) {
    JsonArray chs = obj[DIM_CH_ATTR];

    for (JsonObject ch : chs) {
        uint16_t newValue = ch[DIM_CH_CUR_ATTR].as<uint16_t>();
        uint8_t id = ch[DIM_CH_ID_ATTR];
        if (UartComm.sendDimmerValue(id, newValue) == UART_COMM_SUCCESS) {
            channels[id].curValue = newValue;
        }
    }
}

void IotDimmer::stateJson(JsonObject &obj) {
    JsonArray chs;

    if (obj.containsKey(DIM_CH_ATTR)) {
        chs = obj[DIM_CH_ATTR];
        for (JsonObject ch : chs) {
            int id = ch[DIM_CH_ID_ATTR];
            ch[DIM_CH_CUR_ATTR] = channels[id].curValue;
        }
    } else {
        chs = obj.createNestedArray(DIM_CH_ATTR);
        for (int i = 0; i < DIM_CH_CNT; i++) {
            JsonObject ch = chs.createNestedObject();
            ch[DIM_CH_ID_ATTR] = i;
            ch[DIM_CH_CUR_ATTR] = channels[i].curValue;
        }
    }
}

void IotDimmer::stateFromJson(DimmerState &state, JsonObject &obj) {
    JsonArray chs = obj[DIM_CH_ATTR];
    for (JsonObject ch : chs) {
        int i = ch[DIM_CH_ID_ATTR];
        state.values[i] = ch[DIM_CH_CUR_ATTR];
    }
}

void IotDimmer::stateToJson(DimmerState &state, JsonObject &obj) {
    JsonArray chs = obj.createNestedArray(DIM_CH_ATTR);
    for (int i = 0; i < DIM_CH_CNT; i++) {
        JsonObject ch = chs.createNestedObject();
        ch[DIM_CH_ID_ATTR] = i;
        ch[DIM_CH_CUR_ATTR] = state.values[i];
    }
}