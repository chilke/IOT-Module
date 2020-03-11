#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

class IotWebServer : public ESP8266WebServer {
public:
    IotWebServer(int port);
    void debug();
    String methodName();

private:
    ESP8266HTTPUpdateServer httpUpdater;
};

extern IotWebServer WebServer;

#endif