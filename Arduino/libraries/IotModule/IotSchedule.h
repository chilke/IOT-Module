#ifndef IOT_SCHEDULE_H
#define IOT_SCHEDULE_H

#include <time.h>

#include <ArduinoJson.h>

class IotSchedule {
public:
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t dayMask;
    uint8_t id;
    DeviceState state;

    IotSchedule();

    uint32_t nextTime(tm &curTm, uint32_t curCompTime);
    uint32_t lastTime(tm &curTm, uint32_t curCompTime);

    void fromJson(JsonObject &obj);
    void toJson(JsonObject &obj);

    bool isActive();
    void inActivate();

    void debug();
};

#endif