#ifndef IOT_SCHEDULER_H
#define IOT_SCHEDULER_H

#include <time.h>
#include <ArduinoJson.h>

#include <IotModule.h>

#define MAX_SCHEDULES 5

class IotScheduler {
public:
    IotScheduler();
    void init();
    void addSchedule(JsonObject &obj);
    void deleteSchedule(int id);
    bool getSchedule(int id, JsonObject &obj);
    void handle();

    void debugSchedule(int id);

    bool needsSync;
private:
    void persist();
    void addSchedule(JsonObject &obj, bool needsPersist);
    void updateNextSchedule(tm &curTm, uint32_t curCompTime);

    uint32_t lastHandleTime;
    bool needsRecalc;

    uint32_t nextScheduleTime;

    IotSchedule schedules[MAX_SCHEDULES];
};

extern IotScheduler Scheduler;

#endif