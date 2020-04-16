#include <Arduino.h>
#include <FS.h>
#include <limits.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotScheduler.h>

IotScheduler::IotScheduler() {
    needsRecalc = true;
    lastHandleTime = 0;

    for (int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].id = i;
    }
}

void IotScheduler::init() {
    if (SPIFFS.exists("/schedules.json")) {
        File f = SPIFFS.open("/schedules.json", "r");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);

        while (true) {
            DeserializationError err = deserializeJson(doc, f);

            if (!err) {
                JsonObject obj = doc.as<JsonObject>();
                addSchedule(obj, false);
            } else {
                break;
            }
            doc.clear();
        }

        f.close();
    }

    lastHandleTime = Device.lastStateUpdate;
}

void IotScheduler::updateNextSchedule(tm &curTm, uint32_t curCompTime) {
    nextScheduleTime = ULONG_MAX;

    for (int i = 0; i < MAX_SCHEDULES; i++) {
        uint32_t curNextSchedule = schedules[i].nextTime(curTm, curCompTime);

        if (curNextSchedule < nextScheduleTime) {
            nextScheduleTime = curNextSchedule;
        }
    }

    Logger.debugf("Next schedule updated to %i", nextScheduleTime);
}

void IotScheduler::recalc() {
    needsRecalc = true;
}

void IotScheduler::handle() {
    if (Time.isSet()) {
        time_t curEpochTime = time(nullptr);
        tm curTm;
        localtime_r(&curEpochTime, &curTm);
        uint32_t curCompTime = Time.tmToCompTime(curTm);

        if (needsRecalc || curCompTime < lastHandleTime || curCompTime >=nextScheduleTime) {
            Logger.debug("Scheduler recalculating");

            nextScheduleTime = 0;
            int scheduleId = 0;

            for (int i = 0; i < MAX_SCHEDULES; i++) {
                uint32_t curLastSchedule = schedules[i].lastTime(curTm, curCompTime);

                if (curLastSchedule > nextScheduleTime) {
                    nextScheduleTime = curLastSchedule;
                    scheduleId = i;
                }
            }

            if (nextScheduleTime != 0 && nextScheduleTime > lastHandleTime) {
                Logger.debugf("Executing schedule: %i", scheduleId);
                Device.updateState(schedules[scheduleId].state);
            }

            updateNextSchedule(curTm, curCompTime);
        }

        lastHandleTime = curCompTime;
    }
}

void IotScheduler::persist() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    File f = SPIFFS.open("/schedules.json", "w");
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        JsonObject obj = doc.to<JsonObject>();
        if (getSchedule(i, obj)) {
            serializeJson(obj, f);
        }
        doc.clear();
    }
    f.close();
}

void IotScheduler::addSchedule(JsonObject &obj) {
    addSchedule(obj, true);
}

void IotScheduler::addSchedule(JsonObject &obj, bool needsPersist) {
    int i = 0;

    while (i < MAX_SCHEDULES && schedules[i].isActive()) {
        i++;
    }

    if (i >= MAX_SCHEDULES) {
        return;
    }

    schedules[i].fromJson(obj);

    if (needsPersist) {
        persist();
        recalc();
    }
}

void IotScheduler::deleteSchedule(int id) {
    if (id < MAX_SCHEDULES && schedules[id].isActive()) {
        schedules[id].inActivate();

        persist();
        recalc();
    }
}

bool IotScheduler::getSchedule(int id, JsonObject &obj) {
    if (schedules[id].isActive()) {
        schedules[id].toJson(obj);
        return true;
    }
    return false;
}

void IotScheduler::debugSchedule(int id) {
    schedules[id].debug();
}

IotScheduler Scheduler;