#include <IotModule.h>

void IotDimmerChannel::fromJson(JsonObject &obj) {
    name = obj[DIM_CH_NAME_ATTR].as<String>();
    maxValue = obj[DIM_CH_MAX_ATTR].as<uint16_t>();
    minValue = obj[DIM_CH_MIN_ATTR].as<uint16_t>();
    id = obj[DIM_CH_ID_ATTR].as<uint8_t>();
    if (obj.containsKey(DIM_CH_CUR_ATTR)) {
        curValue = obj[DIM_CH_CUR_ATTR].as<uint16_t>();
        if (UartComm.sendDimmerValue(id, curValue) != UART_COMM_SUCCESS) {
            curValue = 0;
        }
    } else {
        curValue = 0;
    }
    
    isSwitch = obj[DIM_CH_SWITCH_ATTR].as<bool>();
}

void IotDimmerChannel::toJson(JsonObject &obj) {
    obj[DIM_CH_NAME_ATTR] = name;
    obj[DIM_CH_MAX_ATTR] = maxValue;
    obj[DIM_CH_MIN_ATTR] = minValue;
    obj[DIM_CH_CUR_ATTR] = curValue;
    obj[DIM_CH_ID_ATTR] = id;
    obj[DIM_CH_SWITCH_ATTR] = isSwitch;
}

bool IotDimmerChannel::updateState(JsonObject &obj) {
    uint16_t newValue = obj[DIM_CH_CUR_ATTR].as<uint16_t>();
    if (newValue != curValue) {
        if (UartComm.sendDimmerValue(id, newValue) == UART_COMM_SUCCESS) {
            curValue = newValue;
            return true;
        }
    }

    return false;
}

int IotDimmerChannel::idFromJson(JsonObject &obj) {
    return obj[DIM_CH_ID_ATTR];
}