#ifndef IOT_TIME_H
#define IOT_TIME_H

#include <Arduino.h>
#include <WiFiUDP.h>

#define NTP_SYNC_DELAY 60000

#define NTP_WAIT_DELAY 200
#define NTP_WAIT_LOOP 50

#define NTP_HOST_0 "time.nist.gov"
#define NTP_HOST_1 "pool.ntp.org"
#define NTP_HOST_2 "time.google.com"

#define NTP_TO_EPOCH 2208988800UL

typedef struct sntp_msg_t {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_identifier;
    uint32_t reference_timestamp[2];
    uint32_t originate_timestamp[2];
    uint32_t receive_timestamp[2];
    uint32_t transmit_timestamp[2];
} sntp_msg;

class RealTime {
public:
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekDay;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint32_t epochSeconds;
};

class IotTime {
public:
    IotTime();
    void handle();
    void setRealTime(RealTime& rt);
    String toString();

private:
    uint32_t doNtpRequest(uint32_t m);
    uint32_t epochSeconds();
    uint32_t epochSeconds(uint32_t sysSeconds);
    uint32_t sysSeconds();

    uint32_t milliRolls;
    uint32_t lastMillis;

    uint32_t sysToEpochOffset;
    uint32_t nextEpochSecondHold;

    uint32_t nextSync;
    uint8_t curHost;
    int8_t curDrift;
    RealTime rt;

    WiFiUDP udp;
};

extern IotTime Time;

#endif