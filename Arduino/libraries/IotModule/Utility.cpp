#include "Utility.h"

int parseInt(char *buf, int &i) {
  int ret = 0;
  while (buf[i] == ' ' || buf[i] == '\t') {
    ++i;
  }
  while (buf[i] >= '0' && buf[i] <= '9') {
    ret *= 10;
    ret += buf[i] - '0';
    ++i;
  }
  return ret;
}

String parseNonSpaceString(char *buf, int &i) {
  while (isspace(buf[i])) {
    ++i;
  }
  String ret = "";
  while (buf[i] != '\0' && !isspace(buf[i])) {
    ret += buf[i];
    ++i;
  }
  return ret;
}

