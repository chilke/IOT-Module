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
        ch[DIM_CH_CUR_ATTR] = channels[i].curValue;
        i++;
    }
}

void IotDimmer::updateState(DimmerState &state) {
    for (uint8_t id = 0; id < DIM_CH_CNT; id++) {
        if (UartComm.sendDimmerValue(id, state.values[id]) == UART_COMM_SUCCESS) {
            channels[id].curValue = state.values[id];
        }
    }
}

void IotDimmer::updateState(JsonObject &obj) {
    DimmerState state;
    stateFromJson(state, obj);
    updateState(state);
}

void IotDimmer::stateJson(JsonObject &obj) {
    DimmerState state;
    for (int i = 0; i < DIM_CH_CNT; i++) {
        state.values[i] = channels[i].curValue;
    }

    stateToJson(state, obj);
}

void IotDimmer::stateFromJson(DimmerState &state, JsonObject &obj) {
    JsonArray cvs = obj[DIM_CH_CURS_ATTR];
    int i = 0;
    for (JsonVariant val : cvs) {
        state.values[i] = val.as<uint16_t>();
        i++;
    }
}

void IotDimmer::stateToJson(DimmerState &state, JsonObject &obj) {
    JsonArray cvs = obj.createNestedArray(DIM_CH_CURS_ATTR);
    for (int i = 0; i < DIM_CH_CNT; i++) {
        cvs.add(state.values[i]);
    }
}