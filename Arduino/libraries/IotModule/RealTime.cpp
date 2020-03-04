#include <Arduino.h>

#include <RealTime.h>

static uint8_t daysPerMonth[2][13] = {
    {0,31,28,31,30,31,30,31,31,30,31,30,31},
    {0,31,29,31,30,31,30,31,31,30,31,30,31}
};

RealTime::RealTime() {
    lastSeconds = 0;
    offset = 0;

    year = 1970;
    month = 1;
    day = 1;
    hour = 0;
    minute = 0;
    second = 0;
    weekDay = WEEKDAY_THURSDAY;
}

void RealTime::setOffset(int32_t offset) {
    this->offset = offset;
}

int RealTime::toBuffer(uint32_t epochSeconds, uint16_t milliSeconds, char *buf, int size) {
    updateTime(epochSeconds);

    return snprintf(buf, size, "%02u/%02u/%04u %02u:%02u:%02u.%03u", 
        month, day, year, hour, minute, second, milliSeconds);
}

void RealTime::updateTime(uint32_t epochSeconds) {
    epochSeconds += offset;
    int32_t diff = epochSeconds-lastSeconds;
    lastSeconds = epochSeconds;

    if (diff < 0 || diff > 195) { //195 because we could overrun the uint8 second field
        year = 2001;
        month = 1;
        day = 1;
        hour = 0;
        minute = 0;
        second = 0;

        uint32_t days = epochSeconds / SECONDS_PER_DAY;
        epochSeconds -= days*SECONDS_PER_DAY;
        calcDate(days);
        calcTime(epochSeconds);
    } else {
        second += diff;
        if (second >= 60) {
            do {
                minute++;
                second -= 60;
            } while (second >= 60);

            if (minute >= 60) {
                hour++;
                minute -= 60;

                if (hour >= 24) {
                    hour = 0;
                    day++;

                    if (day > daysPerMonth[isLeapYear()][month]) {
                        day = 1;
                        month++;

                        if (month > 12) {
                            month = 1;
                            year++;
                        }
                    }
                }
            }
        }
    }
}

void RealTime::calcDate(uint32_t days) {
    days -= NEW_YEARS_2001_OFFSET;

    weekDay = WEEKDAY_MONDAY + (days%DAYS_PER_WEEK);
    if (weekDay > MAX_WEEKDAY) {
        weekDay -= DAYS_PER_WEEK;
    }

    days = calcYear(days);
    calcMonthAndDay(days);
}

uint32_t RealTime::calcYear(uint32_t days) {
    while (days >= DAYS_PER_QUAD_CENTURY) {
        year += YEARS_PER_QUAD_CENTURY;
        days -= DAYS_PER_QUAD_CENTURY;
    }

    while (days >= DAYS_PER_CENTURY) {
        year += YEARS_PER_CENTURY;
        days -= DAYS_PER_CENTURY;
    }

    while (days >= DAYS_PER_MULTI_QUAD_YEAR) {
        year += YEARS_PER_MULTI_QUAD_YEAR;
        days -= DAYS_PER_MULTI_QUAD_YEAR;
    }

    while (days >= DAYS_PER_QUAD_YEAR) {
        year += YEARS_PER_QUAD_YEAR;
        days -= DAYS_PER_QUAD_YEAR;
    }

    while (days >= DAYS_PER_YEAR) {
        year++;
        days -= DAYS_PER_YEAR;
    }

    return days;
}

void RealTime::calcMonthAndDay(uint32_t days) {
    uint8_t leap = isLeapYear();
    uint8_t i = 1;

    while (days >= daysPerMonth[leap][i]) {
        days -= daysPerMonth[leap][i];
        i++;
    }

    month = i;
    day = days+1;
}

void RealTime::calcTime(uint32_t seconds) {
    while (seconds >= 5*3600) {
        hour += 5;
        seconds -= 5*3600;
    }

    while (seconds >= 3600) {
        hour++;
        seconds -= 3600;
    }

    while (seconds >= 16*60) {
        minute += 16;
        seconds -= 16*60;
    }

    while (seconds >= 4*60) {
        minute += 4;
        seconds -= 4*60;
    }

    while (seconds >= 60) {
        minute++;
        seconds -= 60;
    }

    second = seconds;
}

uint8_t RealTime::isLeapYear() {
    if ((year & 3) == 0) {
        if ((year % 100) == 0) {
            if ((year % 400) != 0) {
                return 0;
            }
        }
        return 1;
    }

    return 0;
}