#ifndef IOT_TIME_H
#define IOT_TIME_H

#include <Arduino.h>


#define NTP_HOST_0 "time.nist.gov"
#define NTP_HOST_1 "pool.ntp.org"
#define NTP_HOST_2 "time.google.com"

class IotTime {
public:
    IotTime();
    void setTz(String tz);
    int curTimeToBuffer(char *buf, int size);
    bool isSet();
    void timeSet();
private:
    bool set;
};

extern IotTime Time;

#endif