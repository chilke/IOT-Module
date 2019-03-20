#ifndef REAL_TIME_H
#define REAL_TIME_H

#include <Arduino.h>

#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24

#define DAYS_PER_WEEK 7
#define WEEKDAY_MONDAY 0
#define WEEKDAY_THURSDAY 4
#define MAX_WEEKDAY 6

#define NEW_YEARS_2001_OFFSET 11323
#define SECONDS_PER_DAY 86400
#define DAYS_PER_YEAR 365
#define DAYS_PER_QUAD_YEAR 1461
#define YEARS_PER_QUAD_YEAR 4
#define DAYS_PER_MULTI_QUAD_YEAR 7305
#define YEARS_PER_MULTI_QUAD_YEAR 20
#define DAYS_PER_CENTURY 36524
#define YEARS_PER_CENTURY 100
#define DAYS_PER_QUAD_CENTURY 146097
#define YEARS_PER_QUAD_CENTURY 400

class RealTime {
public:
    RealTime();

    void setOffset(int32_t offset);
    int toBuffer(uint32_t epochSeconds, uint16_t milliSeconds, char *buf, int size);

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekDay;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint32_t lastSeconds;
    int32_t offset;

private:
    void updateTime(uint32_t epochSeconds);
    void calcDate(uint32_t days);
    uint32_t calcYear(uint32_t days);
    void calcMonthAndDay(uint32_t days);
    void calcTime(uint32_t seconds);
    uint8_t isLeapYear();
};

#endif