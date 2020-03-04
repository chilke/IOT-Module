
#include <IotUtility.h>

void swapSerial() {
    Serial.flush();
    Serial.swap();
}

uint32_t htoi32n(char *hex, int count) {
    uint32_t ret = 0;

    for (int i = 0; i < count; i++) {
        ret <<= 4;
        if (hex[i] >= '0' && hex[i] <= '9') {
            ret += hex[i] - '0';
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            ret += hex[i] - 'a'+10;
        } else {
            ret += hex[i] - 'A'+10;
        }
    }

    return ret;
}

uint16_t htoi16n(char *hex, int count) {
    return (uint16_t)htoi32n(hex, count);
}

uint8_t htoi8n(char *hex, int count) {
    return (uint8_t)htoi32n(hex, count);
}