#include <IotModule.h>

void IotDimmerChannel::fromJson(JsonObject &obj) {
    name = obj[DIM_CH_NAME_ATTR].as<String>();
    maxValue = obj[DIM_CH_MAX_ATTR].as<uint16_t>();
    minValue = obj[DIM_CH_MIN_ATTR].as<uint16_t>();
    id = obj[DIM_CH_ID_ATTR].as<uint8_t>();
    isSwitch = obj[DIM_CH_SWITCH_ATTR].as<bool>();
}

void IotDimmerChannel::toJson(JsonObject &obj) {
    obj[DIM_CH_NAME_ATTR] = name;
    obj[DIM_CH_MAX_ATTR] = maxValue;
    obj[DIM_CH_MIN_ATTR] = minValue;
    obj[DIM_CH_ID_ATTR] = id;
    obj[DIM_CH_SWITCH_ATTR] = isSwitch;
}