#ifndef IOT_SCHEDULE_H
#define IOT_SCHEDULE_H

#include <time.h>

#include <ArduinoJson.h>

#define SCHEDULE_STATE_ATTR "st"
#define SCHEDULE_TIME_ATTR "tm"
#define SCHEDULE_DATE_ATTR "dt"
#define SCHEDULE_DAYS_ATTR "ds"
#define SCHEDULE_HOUR_ATTR "hr"
#define SCHEDULE_MINUTE_ATTR "mn"
#define SCHEDULE_MONTH_ATTR "mo"
#define SCHEDULE_DAY_ATTR "dy"
#define SCHEDULE_YEAR_ATTR "yr"
#define SCHEDULE_ID_ATTR "id"

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