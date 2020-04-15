#include <time.h>
#include <TZ.h>
#include <coredecls.h>

#include <IotModule.h>
#include <IotTime.h>

void todSet() {
    Logger.debug("todSet()");
    Time.timeSet();
}

IotTime::IotTime() {
    settimeofday_cb(todSet);
    setTz("GMT0");
    set = false;
}

void IotTime::setTz(String tz) {
    configTime(tz.c_str(), NTP_HOST_0, NTP_HOST_1, NTP_HOST_2);
    Scheduler.recalc();
}

void IotTime::timeSet() {
    set = true;
}

bool IotTime::isSet() {
    return set;
}

int IotTime::curTimeToBuffer(char *buf, int size) {
    time_t now = time(nullptr);
    tm localTm;
    localtime_r(&now, &localTm);
    return printTime(buf, size, localTm);
}

bool IotTime::parseTime(const char *s, tm &t) {
    int c = sscanf(s, "%d/%d/%d %d:%d:%d", &t.tm_mon, &t.tm_mday, &t.tm_year,
        &t.tm_hour, &t.tm_min, &t.tm_sec);
    if (c != 6) {
        return false;
    }
    t.tm_mon -= 1;
    t.tm_year -= 1900;

    return true;
}

int IotTime::printTime(char *buf, int size, tm &t) {
    return snprintf(buf, size, "%02d/%02d/%04d %02d:%02d:%02d",
        t.tm_mon+1, t.tm_mday, t.tm_year+1900,
        t.tm_hour, t.tm_min, t.tm_sec);
}

uint32_t IotTime::tmToInt(tm &t) {
    uint32_t u;

    u = t.tm_year;
    u *= 12;
    u += t.tm_mon;
    u *= 31;
    u += t.tm_mday;
    u *= 24;
    u += t.tm_hour;
    u *= 60;
    u += t.tm_min;
    u *= 60;
    u += t.tm_sec;

    return u;
}

int IotTime::compareTm(tm &t1, tm &t2) {
    uint32_t u1 = tmToInt(t1);
    uint32_t u2 = tmToInt(t2);

    if (u1 > u2) {
        return 1;
    } else if (u1 < u2) {
        return -1;
    } else {
        return 0;
    }
}

IotTime Time;