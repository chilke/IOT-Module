#include <Arduino.h>
#include <time.h>

#include <IotModule.h>
#include <Schedule.h>

IotSchedule::IotSchedule() {
    year = 0;
    dayMask = 0;
}

bool IotSchedule::isActive() {
    return (year != 0 || dayMask != 0);
}

void IotSchedule::inActivate() {
    year = 0;
    dayMask = 0;
}

void IotSchedule::fromJson(JsonObject &obj) {
    if (obj.containsKey(SCHEDULE_DAYS_ATTR)) {
        JsonArray days = obj[SCHEDULE_DAYS_ATTR].as<JsonArray>();

        for (uint8_t d : days) {
            if (d < 7) {
                dayMask |= 1<<d;
            }
        }
    } else {
        uint16_t y = obj[SCHEDULE_YEAR_ATTR];
        year = y - 1900;
        month = obj[SCHEDULE_MONTH_ATTR].as<uint8_t>();
        day = obj[SCHEDULE_DAY_ATTR].as<uint8_t>();
    }

    hour = obj[SCHEDULE_HOUR_ATTR].as<uint8_t>();
    minute = obj[SCHEDULE_MINUTE_ATTR].as<uint8_t>();

    JsonObject s = obj[SCHEDULE_STATE_ATTR];

    Device.stateFromJson(state, s);
}

void IotSchedule::toJson(JsonObject &obj) {
    if (year != 0) {
        obj[SCHEDULE_YEAR_ATTR] = (uint16_t)year + 1900;
        obj[SCHEDULE_MONTH_ATTR] = month;
        obj[SCHEDULE_DAY_ATTR] = day;
    } else {
        JsonArray days = obj.createNestedArray(SCHEDULE_DAYS_ATTR);
        uint8_t m = 1;
        for (int d = 0; d < 7; d++) {
            if ((dayMask & m) != 0) {
                days.add(d);
            }
            m <<= 1;
        }
    }

    obj[SCHEDULE_HOUR_ATTR] = hour;
    obj[SCHEDULE_MINUTE_ATTR] = minute;
    obj[SCHEDULE_ID_ATTR] = id;

    JsonObject s = obj.createNestedObject(SCHEDULE_STATE_ATTR);
    Device.stateToJson(state, s);
}

uint32_t IotSchedule::nextTime(tm &curTm, uint32_t curCompTime) {
    tm localTime;
    uint8_t daysOff = 0;
    if (year != 0) {
        localTime.tm_year = year;
        localTime.tm_mon = month-1;
        localTime.tm_mday = day;
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else if (dayMask != 0){
        //First check if today is on the schedule
        uint8_t curMask = 1<<curTm.tm_wday;
        daysOff = 255;
        if (dayMask & curMask) {
            if (hour > curTm.tm_hour || (hour == curTm.tm_hour && minute > curTm.tm_min)) {
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
            } while ((dayMask & curMask) == 0);
        }

        localTime.tm_year = curTm.tm_year;
        localTime.tm_mon = curTm.tm_mon;
        localTime.tm_mday = curTm.tm_mday + daysOff;
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else {
        return ULONG_MAX;
    }

    //With daysOff change, we could have gone past end of month
    if (daysOff != 0) {
        time_t t = mktime(&localTime);
        localtime_r(&t, &localTime);
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
    }

    uint32_t t = Time.tmToCompTime(localTime);

    if (t <= curCompTime) {
        return ULONG_MAX;
    }

    return t;
}

uint32_t IotSchedule::lastTime(tm &curTm, uint32_t curCompTime) {
    tm localTime;
    uint8_t daysOff = 0;
    if (year != 0) {
        localTime.tm_year = year;
        localTime.tm_mon = month-1;
        localTime.tm_mday = day;
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else if (dayMask != 0){
        //First check if today is on the schedule
        uint8_t curMask = 1<<curTm.tm_wday;
        daysOff = 255;
        if (dayMask & curMask) {
            if (hour < curTm.tm_hour || (hour == curTm.tm_hour && minute <= curTm.tm_min)) {
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
            } while ((dayMask & curMask) == 0);
        }

        localTime.tm_year = curTm.tm_year;
        localTime.tm_mon = curTm.tm_mon;
        localTime.tm_mday = curTm.tm_mday - daysOff;
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
        localTime.tm_sec = 0;
        localTime.tm_isdst = -1;
    } else {
        return 0;
    }

    //With daysOff change, we could have gone negative
    if (daysOff != 0) {
        time_t t = mktime(&localTime);
        localtime_r(&t, &localTime);
        localTime.tm_hour = hour;
        localTime.tm_min = minute;
    }

    uint32_t t = Time.tmToCompTime(localTime);

    if (t > curCompTime) {
        return 0;
    }

    return t;
}

void IotSchedule::debug() {
    if (year != 0) {
        Logger.debugf("Schedule: %02i/%02i/%02i %02i:%02i", month, day,
            year+1900, hour, minute);
    } else if (dayMask != 0) {
        String days = "[";
        uint8_t curMask = 1;
        uint8_t curDay = 0;
        while (curDay < 7) {
            if (dayMask & curMask) {
                if (days.length() > 1) {
                    days += ",";
                }
                days += String(curDay);
            }
            curMask <<= 1;
            curDay++;
        }
        days += "]";
        Logger.debugf("Schedule: %s %02i:%02i", days.c_str(), hour, minute);
    } else {
        Logger.debug("Inactive schedule");
    }
}