#include <Arduino.h>
#include <IotTime.h>
#include <IotLogger.h>

//TODO add logic to disconnect when tcp logging is disabled
//TODO add logic to reconnect when ip or port is updated
//TODO move initial connect logic to within enableLog, setIP, setTcpPort functions

void IotLogger::init() {
    
}

void IotLogger::begin(unsigned int level, unsigned int enabledLogs) {
    loggerLevel = level;
    this->enabledLogs = enabledLogs;
    ip = IPADDR_NONE;
    tcpPort = 54321;

    if (enabledLogs & LOG_UART) {
        Serial.println();
    }
}

void IotLogger::setIP(IPAddress ip) {
    this->ip = ip;
}

void IotLogger::setTcpPort(uint16_t port) {
    tcpPort = port;
}

void IotLogger::enableLog(unsigned int log) {
    enabledLogs |= log;
}

void IotLogger::disableLog(unsigned int log) {
    enabledLogs &= ~log;
}

void IotLogger::dump(uint8_t *d, uint size) {
    if (loggerLevel >= LOG_LEVEL_DEBUG && enabledLogs != 0) {
        uint i = 0;
        char line[32]; //3*8 for data + 6 for address + 1 for trailing null
        char *p;

        while (i < size) {
            p = &line[0];
            p += sprintf(p, "%04X:", i);
            for (int j = 0; j < 8; j++) {
                if (i >= size) {
                    break;
                }
                p += sprintf(p, " %02X", *(d++));
                i++;
            }

            Logger.debug(line);
        }
    }
}

void IotLogger::errorf(const char *f, ...) {
    va_list args;
    if (loggerLevel >= LOG_LEVEL_ERROR && enabledLogs != 0) {
        va_start(args, f);
        logf("ERROR", f, args);
        va_end(args);
    }
}

void IotLogger::error(const char *s) {
    errorf("%s", s);
}

void IotLogger::warnf(const char *f, ...) {
    va_list args;
    if (loggerLevel >= LOG_LEVEL_WARN && enabledLogs != 0) {
        va_start(args, f);
        logf("WARN", f, args);
        va_end(args);
    }
}

void IotLogger::warn(const char *s) {
    warnf("%s", s);
}

void IotLogger::infof(const char *f, ...) {
    va_list args;
    if (loggerLevel >= LOG_LEVEL_INFO && enabledLogs != 0) {
        va_start(args, f);
        logf("INFO", f, args);
        va_end(args);
    }
}

void IotLogger::info(const char *s) {
    infof("%s", s);
}

void IotLogger::debugf(const char *f, ...) {
    va_list args;
    if (loggerLevel >= LOG_LEVEL_DEBUG && enabledLogs != 0) {
        va_start(args, f);
        logf("DEBUG", f, args);
        va_end(args);
    }
}

void IotLogger::debug(const char *s) {
    debugf("%s", s);
}

void IotLogger::logf(const char *levelName, const char *f, va_list args) {
    char *buffer = new char[LOGGER_BUFLEN+1];
    int curLen;

    if (!buffer) {
        return;
    }

    curLen = getLogHeader(buffer, LOGGER_BUFLEN, levelName);
    // Serial.println("Log Header Length: " + String(curLen));
    // Serial.println(buffer);
    // Serial.println("Remaining Length: " + String(LOGGER_BUFLEN-curLen));
    curLen += vsnprintf(&buffer[curLen], LOGGER_BUFLEN-curLen, f, args);
    // Serial.println("Final log length: " + String(curLen));
    // Serial.println(buffer);

    if (curLen >= LOGGER_BUFLEN) {
        curLen = LOGGER_BUFLEN-1;
    }

    buffer[curLen] = '\n';
    buffer[curLen+1] = '\0';

    write(buffer);

    delete buffer;
}

void IotLogger::write(const char *s) {
    if (enabledLogs & LOG_UART) {
        Serial.print(s);
        yield();
    }

    if (enabledLogs & LOG_TCP && ip != IPADDR_NONE) {
        if (!tcpClient.connected()) {
            tcpClient.connect(ip, tcpPort);
        }

        if (tcpClient.connected()) {
            tcpClient.print(s);
            yield();
        } else {
            Serial.println("Logger failed to connect");
        }
    }
}

int IotLogger::getLogHeader(char *s, int size, const char *levelName) {
    int l = Time.curTimeToBuffer(s, size);

    if (l < size) {
        size -= l;
        s += l;

        return l+snprintf(s, size, " %s: ", levelName);
    }

    return size;
}

IotLogger Logger;