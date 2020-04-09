#include <IotModule.h>

void IotDimmerChannel::updateInfo(JsonObject &obj) {
    name = obj[DIM_CH_NAME_ATTR].as<String>();
    maxValue = obj[DIM_CH_MAX_ATTR].as<uint16_t>();
    minValue = obj[DIM_CH_MIN_ATTR].as<uint16_t>();
    id = obj[DIM_CH_ID_ATTR].as<uint8_t>();
}

void IotDimmerChannel::infoJson(JsonObject &obj) {
    obj[DIM_CH_NAME_ATTR] = name;
    obj[DIM_CH_MAX_ATTR] = maxValue;
    obj[DIM_CH_MIN_ATTR] = minValue;
    obj[DIM_CH_ID_ATTR] = id;
}

void IotDimmerChannel::updateState(JsonObject &obj) {
    uint16_t newValue = obj[DIM_CH_CUR_ATTR].as<uint16_t>();
    if (UartComm.sendDimmerValue(id, newValue) == UART_COMM_SUCCESS) {
        curValue = newValue;
    }
}

void IotDimmerChannel::stateJson(JsonObject &obj) {
    obj[DIM_CH_CUR_ATTR] = curValue;
}

int IotDimmerChannel::idFromJson(JsonObject &obj) {
    return obj[DIM_CH_ID_ATTR];
}