#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <WiFiConnectionManager.h>
#include <IotLogger.h>

#include <IotWebServer.h>
#include <IotWebServerHidden.h>

static String jsonContent = "application/json";
static String textContent = "text/plain";
static String successBody = "{\"status\":\"OK\"}";
static String methodError = "Method not allowed";
static String notFoundError = "Resource not found";

IotWebServer::IotWebServer(int port)
: ESP8266WebServer(port)
{
    on("/listdir", handleListdir);
    on("/credentials", handleCredentials);
    on("/scan_networks", handleScanNetworks);
    on("/network", handleNetwork);
    on("/debug_info", handleDebugInfo);
    on("/logger", handleLogger);
    on("/reset", handleReset);
    on("/upload", handleUpload);
    on("/validate", handleValidate);
    on("/cpu", handleCpu);
    on("/enter", handleEnter);
    on("/exit", handleExit);
    on("/read_device_id", handleReadDeviceId);

    onNotFound(handleNotFound);
}

String IotWebServer::methodName() {
    String ret;

    switch (method()) {
        case HTTP_GET:
        ret = "GET";
        break;
        case HTTP_POST:
        ret = "POST";
        break;
        case HTTP_PUT:
        ret = "PUT";
        break;
        case HTTP_DELETE:
        ret = "DELETE";
        break;
        default:
        ret = "OTHER";
    }

    return ret;
}

void IotWebServer::debug() {
    Logger.debugf("Method: %s URI: %s", WebServer.methodName().c_str(), WebServer.uri().c_str());
    for (int i = 0; i < args(); i++) {
        Logger.debugf("Arg(%i) %s: %s", i, argName(i).c_str(), arg(i).c_str());
    }
}

void sendNotAllowed() {
    WebServer.send(405, textContent, methodError);
}

void sendNotFound() {
    WebServer.send(404, textContent, notFoundError);
}

void handleListdir() {
    Logger.debug("handleListdir()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        String buffer = "";
        buffer.reserve(JSON_BUFFER_SIZE);

        JsonArray& rootArr = jb.createArray();

        Dir dir = SPIFFS.openDir("");

        while (dir.next()) {
            JsonObject& obj = rootArr.createNestedObject();
            obj["name"] = dir.fileName();
            obj["size"] = dir.fileSize();
        }

        rootArr.printTo(buffer);

        WebServer.send(200, jsonContent, buffer);
    } else {
        sendNotAllowed();
    }
}

void handleCredentials() {
    Logger.debug("handleCredentials()");
    WebServer.debug();

    HTTPMethod m = WebServer.method();

    if (m == HTTP_GET) {
        String buffer = "[";
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        JsonObject& obj = jb.createObject();
        buffer.reserve(JSON_BUFFER_SIZE);

        for (int i = 0; i < WCM.savedNetworks(); i++) {
            if (i > 0) {
                buffer += ",";
            }

            obj["ssid"] = WCM.savedSsid(i);
            obj["password"] = WCM.savedPassword(i);

            obj.printTo(buffer);
        }

        buffer += "]";

        WebServer.send(200, jsonContent, buffer);
    } else if (m == HTTP_POST) {
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        JsonObject& obj = jb.parseObject(WebServer.arg("plain"));

        if (obj.success() && obj.containsKey("ssid")) {
            String ssid = obj["ssid"];
            String password = obj["password"];
            String resp = "{\"status\":\"Connect Failed\"}";
            if (WCM.tryConnect(ssid, password)) {
                resp = successBody;
            }

            WebServer.send(200, jsonContent, resp);
        } else {
            WebServer.send(500, textContent, "Error parsing body");
        }
    } else if (m == HTTP_DELETE) {
        String ssid = WebServer.arg("ssid");

        if (ssid.length() > 0) {
            String resp = "{\"status\":\"ssid not found\"}";
            if (WCM.deleteSsid(ssid)) {
                resp = successBody;
            }
            WebServer.send(200, jsonContent, resp);
        } else {
            WebServer.send(500, textContent, "ssid required");
        }
    } else {
        sendNotAllowed();
    }
}

void handleScanNetworks() {
    Logger.debug("handleNetworks()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        WiFiScanInfo *info = WCM.scan();
        WiFiScanInfo *cur = info;
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        String buffer = "";
        buffer.reserve(JSON_BUFFER_SIZE);

        JsonObject& obj = jb.createObject();

        WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
        WebServer.send(200, jsonContent, "");

        WebServer.sendContent("[");

        while (1) {
            obj["ssid"] = cur->ssid;
            obj["bssid"] = cur->bssidStr;
            obj["rssi"] = cur->rssi;
            obj["encrypted"] = (bool)(cur->encType!=ENC_TYPE_NONE);
            obj["channel"] = cur->channel;
            obj["hidden"] = cur->hidden;

            obj.printTo(buffer);

            WebServer.sendContent(buffer);

            cur = cur->next;

            if (cur) {
                WebServer.sendContent(",");
            } else {
                break;
            }

            buffer = "";
        }

        delete info;

        WebServer.sendContent("]");
        WebServer.sendContent("");
    } else {
        sendNotAllowed();
    }
}

void handleNetwork() {
    Logger.debug("handleNetwork()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        String buffer = "[";

        buffer.reserve(JSON_BUFFER_SIZE);

        JsonObject& obj = jb.createObject();

        obj["if"] = "STATION";
        obj["enabled"] = (bool)(WiFi.getMode() & WIFI_STA);
        obj["connected"] = (bool)(WiFi.status() == WL_CONNECTED);
        obj["ssid"] = WiFi.SSID();
        obj["bssid"] = WiFi.BSSIDstr();
        obj["mac"] = WiFi.macAddress();
        obj["rssi"] = WiFi.RSSI();
        obj["ip"] = WiFi.localIP().toString();

        obj.printTo(buffer);

        buffer += ",";

        obj["if"] = "AP";
        obj["enabled"] = (bool)(WiFi.getMode() & WIFI_AP);
        obj.remove("connected");
        obj["ssid"] = WiFi.softAPSSID();
        obj.remove("bssid");
        obj["mac"] = WiFi.softAPmacAddress();
        obj.remove("rssi");
        obj["ip"] = WiFi.softAPIP().toString();
        obj["numConnected"] = WiFi.softAPgetStationNum();

        obj.printTo(buffer);

        buffer += "]";

        WebServer.send(200, jsonContent, buffer);
    } else {
        sendNotAllowed();
    }
}

void handleDebugInfo() {
    Logger.debug("handleDebugInfo()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        JsonObject& obj = jb.createObject();
        String buffer = "";
        uint32_t u32;
        uint16_t u16;
        uint8_t u8;
        FSInfo fsInfo;
        buffer.reserve(JSON_BUFFER_SIZE);

        /* Free, max block, fragmentation */
        ESP.getHeapStats(&u32, &u16, &u8);

        obj["freeStack"] = ESP.getFreeContStack();
        obj["freeHeap"] = u32;
        obj["heapMaxBlock"] = u16;
        obj["heapFragmentation"] = String(u8)+"%";
        obj["version"] = ESP.getFullVersion();
        obj["cpuFreq"] = ESP.getCpuFreqMHz();
        obj["sketchSize"] = ESP.getSketchSize();
        obj["freeSketchSpace"] = ESP.getFreeSketchSpace();
        obj["vcc"] = ESP.getVcc();
        SPIFFS.info(fsInfo);
        JsonObject& obj2 = jb.createObject();
        obj2["totalBytes"] = fsInfo.totalBytes;
        obj2["usedBytes"] = fsInfo.usedBytes;
        obj2["blockSize"] = fsInfo.blockSize;
        obj2["pageSize"] = fsInfo.pageSize;
        obj2["maxOpenFiles"] = fsInfo.maxOpenFiles;
        obj2["maxPathLength"] = fsInfo.maxPathLength;
        obj["fsInfo"] = obj2;

        obj.printTo(buffer);
        WebServer.send(200, jsonContent, buffer);
    } else {
        sendNotAllowed();
    }
}

void handleLogger() {
    Logger.debug("handleLogger()");
    WebServer.debug();

    if (WebServer.method() == HTTP_POST) {
        DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
        JsonObject& obj = jb.parseObject(WebServer.arg("plain"));

        uint16_t port = obj["tcpPort"];
        if (port != 0) {
            Logger.debugf("Setting port: %i", port);
            Logger.setTcpPort(port);
        }
        String ipStr = obj["ip"];
        if (ipStr.length() != 0) {
            IPAddress ip;
            if (ip.fromString(ipStr)) {
                Logger.debugf("Setting IP: %s", ipStr.c_str());
                Logger.setIP(ip);
            }
        }
        if (obj.containsKey("tcp")) {
            if (obj["tcp"]) {
                Logger.enableLog(LOG_TCP);
            } else {
                Logger.disableLog(LOG_TCP);
            }
        }

        WebServer.send(200, jsonContent, successBody);
    } else {
        sendNotAllowed();
    }
}

void handleReset() {
    Logger.debug("handleReset()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        WebServer.send(200, jsonContent, successBody);

        if (WebServer.arg("safe_mode").compareTo("true") == 0) {
            Logger.debug("Writing SAFEMODE command to RTC");
            uint32_t buf[] = RESET_SAFEMODE_DATA;
            if (!ESP.rtcUserMemoryWrite(RESET_SAFEMODE_ADDR, buf, RESET_SAFEMODE_SIZE*4)) {
                Logger.debug("RTC Write Failed??");
            }
        }

        delay(100);

        ESP.restart();
    } else {
        sendNotAllowed();
    }
}

void handleUpload() {
    Logger.debug("handleUpload()");
    WebServer.debug();

    HTTPMethod m = WebServer.method();

    if (m == HTTP_POST || m == HTTP_PUT) {
        String path = WebServer.arg("file_name");
        String contents = WebServer.arg("plain");

        if (!path.length()) {
            WebServer.send(400, textContent, "Missing file_name");
        } else if (!contents.length()) {
            WebServer.send(400, textContent, "Missing body");
        } else {
            if (SPIFFS.exists(path)) {
                if (m == HTTP_PUT) {
                    if (!SPIFFS.remove(path)) {
                        WebServer.send(500, textContent, "Error deleting existing file");
                        return;
                    }
                } else {
                    WebServer.send(400, textContent, "Resource already exists, try PUT to update");
                    return;
                }
            }

            File f = SPIFFS.open(path, "w");
            if (f.print(contents) != contents.length()) {
                WebServer.send(500, textContent, "Error writing file");
            } else {
                WebServer.send(200, jsonContent, successBody);
            }

            f.close();
        }
    } else {
        sendNotAllowed();
    }
}

void handleValidate() {
    Logger.debug("handleValidate()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET && WebServer.hasArg("file")) {
        String fileName = WebServer.arg("file");
        if (SPIFFS.exists(fileName)) {
            File f = SPIFFS.open(fileName, "r");
            int ret = PicUpdater.init(&f);
            WebServer.send(200, textContent, String(ret));
        } else {
            sendNotFound();
        }
    } else {
        sendNotAllowed();
    }
}

void handleCpu() {
    Logger.debug("handleCpu()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET && WebServer.hasArg("freq")) {
        int freq = WebServer.arg("freq").toInt();

        if (freq == 80 || freq == 160) {
            system_update_cpu_freq(freq);

            WebServer.send(200, textContent, "Done");
        } else {
            sendNotAllowed();
        }
    } else {
        sendNotAllowed();
    }
}

void handleEnter() {
    Logger.debug("handleEnter()");
    WebServer.debug();

    PicUpdater.enterProgramMode();

    WebServer.send(200, textContent, "Done");
}

void handleExit() {
    Logger.debug("handleExit()");
    WebServer.debug();

    PicUpdater.exitProgramMode();

    WebServer.send(200, textContent, "Done");
}

void handleReadDeviceId() {
    uint16_t deviceId;
    uint16_t revisionId;
    char cStr[5];
    Logger.debug("handleReadDeviceId()");
    WebServer.debug();

    PicUpdater.getDeviceAndRevisionId(&deviceId, &revisionId);

    DynamicJsonBuffer jb(JSON_BUFFER_SIZE);
    JsonObject& obj = jb.createObject();
    String buffer = "";
    buffer.reserve(JSON_BUFFER_SIZE);

    obj["deviceInt"] = deviceId;
    sprintf(cStr, "%04X", deviceId);
    obj["deviceId"] = String(cStr);

    obj["revisionInt"] = revisionId;
    sprintf(cStr, "%04X", revisionId);
    obj["revisionId"] = String(cStr);

    obj.printTo(buffer);
    WebServer.send(200, jsonContent, buffer);
}

void handleNotFound() {
    Logger.debug("handleNotFound()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        if (SPIFFS.exists(WebServer.uri())) {
            File f = SPIFFS.open(WebServer.uri(), "r");
            WebServer.streamFile(f, textContent);
            f.close();
            return;
        }
    } else if (WebServer.method() == HTTP_DELETE) {
        if (SPIFFS.exists(WebServer.uri())) {
            if (SPIFFS.remove(WebServer.uri())) {
                WebServer.send(200, jsonContent, successBody);
            } else {
                WebServer.send(500, textContent, "Error while deleting");
            }
            return;
        }
    }

    sendNotFound();
}

IotWebServer WebServer(80);