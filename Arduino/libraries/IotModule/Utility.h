#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

int parseInt(char *buf, int &i);
String parseNonSpaceString(char *buf, int &i);

#endif