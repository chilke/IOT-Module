#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

enum UploadStatus {
    UPLOAD_INIT, UPLOAD_INPROGRESS, UPLOAD_COMPLETE, UPLOAD_ERROR_EXISTS
};

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