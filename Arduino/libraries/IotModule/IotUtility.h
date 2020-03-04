#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

void swapSerial();
uint32_t htoi32n(char *hex, int count);
uint16_t htoi16n(char *hex, int count);
uint8_t htoi8n(char *hex, int count);

#endif