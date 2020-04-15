#include <Arduino.h>
#include <FS.h>
#include <limits.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotScheduler.h>

IotScheduler::IotScheduler() {
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].year = 0;
        schedules[i].dayMask = 0;
    }
    needsRecalc = true;
    lastHandleTime.tm_year = 0;
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
}

void IotScheduler::updateNextSchedule(tm &curTm, time_t curTime) {
    nextScheduleTime = LONG_MAX;

    for (int i = 0; i < MAX_SCHEDULES; i++) {
        time_t curNextSchedule = nextTime(i, curTm, curTime);

        if (curNextSchedule < nextScheduleTime) {
            nextScheduleTime = curNextSchedule;
            nextScheduleId = i;
        }
    }

    Logger.debugf("Next schedule updated to %i at %i", nextScheduleId, nextScheduleTime);
}

void IotScheduler::recalc() {
    needsRecalc = true;
}

void IotScheduler::handle() {
    if (Time.isSet()) {
        time_t curTime = time(nullptr);
        tm curTm;
        localtime_r(&curTime, &curTm);
        if (needsRecalc) {
            Logger.debug("Scheduler recalculating");
            if (lastHandleTime.tm_year == 0) {
                lastHandleTime = Device.lastStateUpdate;
            }
            //First determine if we've skipped any schedules since device last update time
            nextScheduleTime = 0;
            nextScheduleId = 0;
            for (int i = 0; i < MAX_SCHEDULES; i++) {
                time_t curLastSchedule = lastTime(i, curTm, curTime);

                if (curLastSchedule > nextScheduleTime) {
                    nextScheduleTime = curLastSchedule;
                    nextScheduleId = i;
                }
            }

            if (nextScheduleTime != 0) { // && nextScheduleTime > lastHandleTime) {
                Logger.debugf("Executing skipped schedule: %i", nextScheduleId);
                Device.updateState(schedules[nextScheduleId].state);
            }

            updateNextSchedule(curTm, curTime);
        } else if (curTime >= nextScheduleTime) {
            Logger.debugf("Executing schedule: %i", nextScheduleId);
            Device.updateState(schedules[nextScheduleId].state);
            updateNextSchedule(curTm, curTime);
        }

        lastHandleTime = curTm;
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

    while (i < MAX_SCHEDULES && (schedules[i].year != 0 || schedules[i].dayMask != 0)) {
        i++;
    }

    if (i >= MAX_SCHEDULES) {
        return;
    }

    if (obj.containsKey(SCHEDULE_DAYS_ATTR)) {
        JsonArray days = obj[SCHEDULE_DAYS_ATTR].as<JsonArray>();

        for (uint8_t d : days) {
            if (d < 7) {
                schedules[i].dayMask |= 1<<d;
            }
        }
    } else {
        uint16_t year = obj[SCHEDULE_YEAR_ATTR];
        schedules[i].year = year - 1900;
        schedules[i].month = obj[SCHEDULE_MONTH_ATTR].as<uint8_t>();
        schedules[i].day = obj[SCHEDULE_DAY_ATTR].as<uint8_t>();
    }

    schedules[i].hour = obj[SCHEDULE_HOUR_ATTR].as<uint8_t>();
    schedules[i].minute = obj[SCHEDULE_MINUTE_ATTR].as<uint8_t>();

    JsonObject state = obj[SCHEDULE_STATE_ATTR];

    Device.stateFromJson(schedules[i].state, state);

    if (needsPersist) {
        persist();
        recalc();
    }
}

void IotScheduler::deleteSchedule(int id) {
    if (id < MAX_SCHEDULES && (schedules[id].year != 0 || schedules[id].dayMask != 0)) {
        while (id < MAX_SCHEDULES-1 && (schedules[id+1].year != 0 || schedules[id].dayMask != 0)) {
            memcpy(&schedules[id], &schedules[id+1], sizeof(Schedule));
            id++;
        }
        schedules[id].year = 0;
        schedules[id].dayMask = 0;

        persist();
        recalc();
    }
}

bool IotScheduler::getSchedule(int id, JsonObject &obj) {
    if (schedules[id].year != 0 || schedules[id].dayMask != 0) {
        if (schedules[id].year != 0) {
            obj[SCHEDULE_YEAR_ATTR] = (uint16_t)schedules[id].year + 1900;
            obj[SCHEDULE_MONTH_ATTR] = schedules[id].month;
            obj[SCHEDULE_DAY_ATTR] = schedules[id].day;
        } else {
            JsonArray days = obj.createNestedArray(SCHEDULE_DAYS_ATTR);
            uint8_t m = 1;
            for (int d = 0; d < 7; d++) {
                if ((schedules[id].dayMask & m) != 0) {
                    days.add(d);
                }
                m <<= 1;
            }
        }

        obj[SCHEDULE_HOUR_ATTR] = schedules[id].hour;
        obj[SCHEDULE_MINUTE_ATTR] = schedules[id].minute;
        obj[SCHEDULE_ID_ATTR] = id;

        JsonObject state = obj.createNestedObject(SCHEDULE_STATE_ATTR);
        Device.stateToJson(schedules[id].state, state);

        return true;
    }

    return false;
}

time_t IotScheduler::nextTime(int scheduleId, tm &curTm, time_t curTime) {
    tm localTime;
    if (schedules[scheduleId].year != 0) {
        localTime.tm_year = schedules[scheduleId].year;
        localTime.tm_mon = schedules[scheduleId].month-1;
        localTime.tm_mday = schedules[scheduleId].day;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else if (schedules[scheduleId].dayMask != 0){
        //First check if today is on the schedule
        uint8_t curMask = 1<<curTm.tm_wday;
        uint8_t daysOff = 255;
        if (schedules[scheduleId].dayMask & curMask) {
            if (schedules[scheduleId].hour > curTm.tm_hour || (schedules[scheduleId].hour == curTm.tm_hour && schedules[scheduleId].minute > curTm.tm_min)) {
                daysOff = 0;
            }
        }

        if (daysOff == 255) {
            daysOff = 0;
            do {
                daysOff++;
                curMask <<= 1;
                if (curMask > 1<<6) {
                    curMask = 1;
                }
            } while ((schedules[scheduleId].dayMask & curMask) == 0);
        }

        localTime.tm_year = curTm.tm_year;
        localTime.tm_mon = curTm.tm_mon;
        localTime.tm_mday = curTm.tm_mday + daysOff;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else {
        return LONG_MAX;
    }

    time_t t = mktime(&localTime);

    if (t <= curTime) {
        return LONG_MAX;
    }

    return t;
}

time_t IotScheduler::lastTime(int scheduleId, tm &curTm, time_t curTime) {
    tm localTime;
    if (schedules[scheduleId].year != 0) {
        localTime.tm_year = schedules[scheduleId].year;
        localTime.tm_mon = schedules[scheduleId].month-1;
        localTime.tm_mday = schedules[scheduleId].day;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else if (schedules[scheduleId].dayMask != 0){
        //First check if today is on the schedule
        uint8_t curMask = 1<<curTm.tm_wday;
        uint8_t daysOff = 255;
        if (schedules[scheduleId].dayMask & curMask) {
            if (schedules[scheduleId].hour < curTm.tm_hour || (schedules[scheduleId].hour == curTm.tm_hour && schedules[scheduleId].minute <= curTm.tm_min)) {
                daysOff = 0;
            }
        }

        if (daysOff == 255) {
            daysOff = 0;
            do {
                daysOff++;
                curMask >>= 1;
                if (curMask == 0) {
                    curMask = 1<<6;
                }
            } while ((schedules[scheduleId].dayMask & curMask) == 0);
        }

        localTime.tm_year = curTm.tm_year;
        localTime.tm_mon = curTm.tm_mon;
        localTime.tm_mday = curTm.tm_mday - daysOff;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else {
        return 0;
    }

    time_t t = mktime(&localTime);

    if (t > curTime) {
        return 0;
    }

    return t;
}

void IotScheduler::debugSchedule(int id) {
    if (schedules[id].year != 0) {
        Logger.debugf("Schedule: %02i/%02i/%02i %02i:%02i", schedules[id].month, schedules[id].day,
            schedules[id].year+1900, schedules[id].hour, schedules[id].minute);
    } else if (schedules[id].dayMask != 0) {
        String days = "[";
        uint8_t curMask = 1;
        uint8_t curDay = 0;
        while (curDay < 7) {
            if (schedules[id].dayMask & curMask) {
                if (days.length() > 1) {
                    days += ",";
                }
                days += String(curDay);
            }
            curMask <<= 1;
            curDay++;
        }
        days += "]";
        Logger.debugf("Schedule: %s %02i:%02i", days.c_str(), schedules[id].hour, schedules[id].minute);
    } else {
        Logger.debug("Inactive schedule");
    }
}

IotScheduler Scheduler;