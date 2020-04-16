#ifndef IOT_TIME_H
#define IOT_TIME_H

#include <Arduino.h>
#include <time.h>

#define NTP_HOST_0 "time.nist.gov"
#define NTP_HOST_1 "pool.ntp.org"
#define NTP_HOST_2 "time.google.com"

#define SECS_PER_YEAR 31622400
#define SECS_PER_MONTH 2678400
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600
#define SECS_PER_MINUTE 60

class IotTime {
public:
    IotTime();
    void setTz(String tz);
    int curTimeToBuffer(char *buf, int size);
    bool isSet();
    void timeSet();
    bool parseTime(const char *s, tm &t);
    int printTime(char *buf, int size, tm &t);
    uint32_t curCompTime();
    uint32_t tmToCompTime(tm &t);
    uint32_t epochToCompTime(time_t t);

    void setTime(time_t t);

    int compareTm(tm &t1, tm &t2);
private:
    bool set;
};

extern IotTime Time;

#endif