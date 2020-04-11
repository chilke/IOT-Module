#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotScheduler.h>

IotScheduler::IotScheduler() {
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].month = 0;
        schedules[i].dayMask = 0;
    }
}

void IotScheduler::init() {
    if (SPIFFS.exists("/schedules.json")) {
        File f = SPIFFS.open("/schedules.json", "r");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);

        DeserializationError err = deserializeJson(doc, f);

        if (!err) {
            JsonArray arr = doc.as<JsonArray>();
            for (JsonObject obj : arr) {
                addSchedule(obj, false);
            }
        }

        f.close();
    }
}

void IotScheduler::persist() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonArray arr = doc.to<JsonArray>();
    getSchedules(arr, false);
    File f = SPIFFS.open("/schedules.json", "w");
    serializeJson(arr, f);
    f.close();
}

void IotScheduler::persistState(int id, JsonObject &state) {
    char filename[20];
    sprintf(filename, "/schedule%u.json", id);
    File f = SPIFFS.open(filename, "w");
    serializeJson(state, f);
    f.close();
}

void IotScheduler::loadState(int id, JsonObject &state) {
    char filename[20];
    sprintf(filename, "/schedule%u.json", id);
    File f = SPIFFS.open(filename, "r");
    //state = f.readString();
    f.close();
}

void IotScheduler::addSchedule(JsonObject &obj) {
    addSchedule(obj, true);
}

void IotScheduler::addSchedule(JsonObject &obj, bool needsPersist) {
    int i = 0;

    while (i < MAX_SCHEDULES && (schedules[i].month != 0 || schedules[i].dayMask != 0)) {
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
        schedules[i].month = obj[SCHEDULE_MONTH_ATTR].as<uint8_t>();
        schedules[i].day = obj[SCHEDULE_DAY_ATTR].as<uint8_t>();
    }

    schedules[i].hour = obj[SCHEDULE_HOUR_ATTR].as<uint8_t>();
    schedules[i].minute = obj[SCHEDULE_MINUTE_ATTR].as<uint8_t>();

    if (needsPersist) {
        persist();
    }
}

void IotScheduler::deleteSchedule(int id) {
    if (id < MAX_SCHEDULES) {
        schedules[id].month = 0;
        schedules[id].dayMask = 0;

        persist();
    }
}

void IotScheduler::getSchedules(JsonArray &arr) {
    getSchedules(arr, true);
}

void IotScheduler::getSchedules(JsonArray &arr, bool includeStates) {
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        if (schedules[i].month != 0 || schedules[i].dayMask != 0) {
            JsonObject obj = arr.createNestedObject();
            if (schedules[i].month != 0) {
                obj[SCHEDULE_MONTH_ATTR] = schedules[i].month;
                obj[SCHEDULE_DAY_ATTR] = schedules[i].day;
            } else {
                JsonArray days = obj.createNestedArray(SCHEDULE_DAYS_ATTR);
                uint8_t m = 1;
                for (int d = 0; d < 7; d++) {
                    if ((schedules[i].dayMask & m) != 0) {
                        days.add(d);
                    }
                    m <<= 1;
                }
            }

            obj[SCHEDULE_HOUR_ATTR] = schedules[i].hour;
            obj[SCHEDULE_MINUTE_ATTR] = schedules[i].minute;
            obj[SCHEDULE_ID_ATTR] = i;
        }
    }
}

IotScheduler Scheduler;