

int parseInt(char *buf, int &i) {
  int ret = 0;
  while (buf[i] == ' ' || buf[i] == '\t') {
    Serial.println("parseInt skipping space");
    ++i;
  }
  while (buf[i] >= '0' && buf[i] <= '9') {
    Serial.print("parseInt processing: ");
    Serial.print(buf[i]);
    Serial.print(" at index: ");
    Serial.print(i);
    ret *= 10;
    ret += buf[i] - '0';
    Serial.print(" new value: ");
    Serial.println(ret);
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

