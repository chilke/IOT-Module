#include <time.h>
#include <TZ.h>
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
    configTime(TZ_America_Chicago, NTP_HOST_0, NTP_HOST_1, NTP_HOST_2);
}

void IotTime::timeSet() {
    set = true;
}

bool IotTime::isSet() {
    return set;
}

int IotTime::curTimeToBuffer(char *buf, int size) {
    time_t now = time(nullptr);
    tm* localTm = localtime(&now);
    return snprintf(buf, size, "%02d/%02d/%04d %02d:%02d:%02d",
        localTm->tm_mon+1, localTm->tm_mday, localTm->tm_year+1900,
        localTm->tm_hour, localTm->tm_min, localTm->tm_sec);
}

IotTime Time;