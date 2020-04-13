#ifndef IOT_SCHEDULER_H
#define IOT_SCHEDULER_H

#include <time.h>
#include <ArduinoJson.h>

#include <IotModule.h>

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

#define MAX_SCHEDULES 5

typedef struct schedule_t {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t dayMask;
    DeviceState state;
} Schedule;

class IotScheduler {
public:
    IotScheduler();
    void init();
    void addSchedule(JsonObject &obj);
    void deleteSchedule(int id);
    bool getSchedule(int id, JsonObject &obj);
    void handle();

    time_t nextTime(int scheduleId, tm &curTime);
private:
    void persist();
    void addSchedule(JsonObject &obj, bool needsPersist);

    Schedule schedules[MAX_SCHEDULES];
};

extern IotScheduler Scheduler;

#endif