#include <time.h>
#include <coredecls.h>

#include <IotModule.h>
#include <IotTime.h>

void todSet() {
    Logger.debug("todSet()");
    Time.timeSet();
}

void IotTime::begin() {
    set = false;
    settimeofday_cb(todSet);
    configTime("GMT0", NTP_HOST_0, NTP_HOST_1, NTP_HOST_2);
}

void IotTime::timeSet() {
    set = true;
}

bool IotTime::isSet() {
    return set;
}

int IotTime::curTimeToBuffer(char *buf, int size) {
    return snprintf(buf, size, "Current Time");
}

IotTime Time;