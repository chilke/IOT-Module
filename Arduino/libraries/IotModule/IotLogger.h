#ifndef IOT_LOGGER_H
#define IOT_LOGGER_H

#include <Arduino.h>
#include <WiFiClient.h>

#define LOGGER_BUFLEN 101

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_ALL 4

#define LOG_UART 1
#define LOG_TCP 2

class IotLogger {
public:
    void init();
    void begin(unsigned int level, unsigned int enabledLogs);
    void dump(uint8_t *d, uint size);
    void debug(const char *s);
    void info(const char *s);
    void warn(const char *s);
    void error(const char *s);
    void debugf(const char *f, ...);
    void infof(const char *f, ...);
    void warnf(const char *f, ...);
    void errorf(const char *f, ...);
    void setIP(IPAddress ip);
    void setPort(uint16_t port);
    void enableLog(unsigned int log);
    void disableLog(unsigned int log);
    void setLevel(String level);
    void persist();

private:
    void logf(const char *levelName, const char *f, va_list args);
    void write(const char *s);
    int getLogHeader(char *s, int size, const char* levelName);
    uint8_t loggerLevel;
    uint8_t enabledLogs;
    IPAddress ip;
    uint16_t port;
    WiFiClient tcpClient;
};

extern IotLogger Logger;

#endif //IOT_LOGGER_H