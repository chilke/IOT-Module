#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <IotModule.h>
#include <IotLogger.h>

void IotLogger::init() {
    loggerLevel = LOG_LEVEL_DEBUG;
    enabledLogs = LOG_UART;
    ip = IPADDR_NONE;
    tcpPort = 54321;
    udpPort = 12345;

    if (SPIFFS.exists("/logger_config.json")) {
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        File f = SPIFFS.open("/logger_config.json", "r");
        DeserializationError err = deserializeJson(doc, f);

        if (!err) {
            loggerLevel = doc["loggerLevel"];
            enabledLogs = doc["enabledLogs"];
            String ipStr = doc["ip"];
            ip.fromString(ipStr);
            tcpPort = doc["tcpPort"];
            udpPort = doc["udpPort"];
        }

        f.close();
    }

    if (enabledLogs & LOG_UART) {
        Serial.println();
    }
}

void IotLogger::persist() {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    doc["loggerLevel"] = loggerLevel;
    doc["enabledLogs"] = enabledLogs;
    doc["ip"] = ip.toString();
    doc["tcpPort"] = tcpPort;
    doc["udpPort"] = udpPort;

    File f = SPIFFS.open("/logger_config.json", "w");
    serializeJson(doc, f);
    f.close();
}

void IotLogger::setIP(IPAddress ip) {
    if (tcpClient.connected()) {
        tcpClient.stop();
    }
    this->ip = ip;
}

void IotLogger::setTcpPort(uint16_t port) {
    if (tcpClient.connected()) {
        tcpClient.stop();
    }
    tcpPort = port;
}

void IotLogger::setUdpPort(uint16_t port) {
    udpPort = port;
}

void IotLogger::enableLog(unsigned int log) {
    enabledLogs |= log;
}

void IotLogger::disableLog(unsigned int log) {
    enabledLogs &= ~log;
}

void IotLogger::setLevel(String level) {
    if (level == "debug") {
        loggerLevel = LOG_LEVEL_DEBUG;
    } else if (level == "info") {
        loggerLevel = LOG_LEVEL_INFO;
    } else if (level == "warn") {
        loggerLevel = LOG_LEVEL_WARN;
    } else if (level == "error") {
        loggerLevel = LOG_LEVEL_ERROR;
    } else if (level == "none") {
        loggerLevel = LOG_LEVEL_NONE;
    }
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
    }

    if (enabledLogs & LOG_TCP && ip != IPADDR_NONE) {
        if (!tcpClient.connected()) {
            tcpClient.connect(ip, tcpPort);
        }

        if (tcpClient.connected()) {
            tcpClient.print(s);
        }
    }

    if (enabledLogs & LOG_UDP && ip != IPADDR_NONE) {
        if (!udpClient.beginPacket(ip, udpPort)) {
            Serial.println("beginPacket failure");
        }
        udpClient.print(s);
        if (!udpClient.endPacket()) {
            Serial.println("endPacket failure");
        }
    }
    yield();
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