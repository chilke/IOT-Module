#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>

class IotWebServer : public ESP8266WebServer {
public:
    IotWebServer(int port);
    void debug();
    String methodName();
};

extern IotWebServer WebServer;

#endif