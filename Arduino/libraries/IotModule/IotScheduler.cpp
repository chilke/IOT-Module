#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotScheduler.h>

IotScheduler::IotScheduler() {
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].year = 0;
        schedules[i].dayMask = 0;
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

time_t IotScheduler::nextTime(int scheduleId, tm &curTime) {
    if (schedules[scheduleId].year != 0) {
        tm localTime;
        localTime.tm_year = schedules[scheduleId].year;
        localTime.tm_mon = schedules[scheduleId].month-1;
        localTime.tm_mday = schedules[scheduleId].day;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;

        return mktime(&localTime);
    } else if (schedules[scheduleId].dayMask != 0){
        //First check if today is on the schedule
        uint8_t curMask = 1<<curTime.tm_wday;
        uint8_t daysOff = 255;
        if (schedules[scheduleId].dayMask & curMask) {
            if (schedules[scheduleId].hour > curTime.tm_hour || (schedules[scheduleId].hour == curTime.tm_hour && schedules[scheduleId].minute > curTime.tm_min)) {
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

        tm localTime;
        localTime.tm_year = curTime.tm_year;
        localTime.tm_mon = curTime.tm_mon;
        localTime.tm_mday = curTime.tm_mday + daysOff;
        localTime.tm_hour = schedules[scheduleId].hour;
        localTime.tm_min = schedules[scheduleId].minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;

        return mktime(&localTime);
    }
    // This will return max value to ensure inactive schedules don't ever fire
    return -1;
}

IotScheduler Scheduler;