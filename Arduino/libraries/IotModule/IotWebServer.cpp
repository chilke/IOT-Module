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
    on("/send_file", handleSendFile);
    on("/cpu", handleCpu);
    on("/read_device_id", handleReadDeviceId);
    on("/read", handleReadMemory);
    on("/send_command", handleSendCommand);
    on("/publish", handlePublish);
    on("/device_info", handleDeviceInfo);

    onNotFound(handleNotFound);

    httpUpdater.setup(this);
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

void sendDone() {
    WebServer.send(200, textContent, "Done");
}

void handleListdir() {
    Logger.debug("handleListdir()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET) {
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        String buffer = "";
        buffer.reserve(JSON_BUFFER_SIZE);

        JsonArray rootArr = doc.to<JsonArray>();

        Dir dir = SPIFFS.openDir("");

        while (dir.next()) {
            JsonObject obj = rootArr.createNestedObject();
            obj["name"] = dir.fileName();
            obj["size"] = dir.fileSize();
        }

        serializeJson(doc, buffer);

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
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        JsonObject obj = doc.to<JsonObject>();
        buffer.reserve(JSON_BUFFER_SIZE);

        for (int i = 0; i < WCM.savedNetworks(); i++) {
            if (i > 0) {
                buffer += ",";
            }

            obj["ssid"] = WCM.savedSsid(i);
            obj["password"] = WCM.savedPassword(i);

            serializeJson(doc, buffer);
        }

        buffer += "]";

        WebServer.send(200, jsonContent, buffer);
    } else if (m == HTTP_POST) {
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        DeserializationError err = deserializeJson(doc, WebServer.arg("plain"));

        if (!err && doc.containsKey("ssid")) {
            String ssid = doc["ssid"];
            String password = doc["password"];
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
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        String buffer = "";
        buffer.reserve(JSON_BUFFER_SIZE);

        JsonObject obj = doc.to<JsonObject>();

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

            serializeJson(doc, buffer);

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
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        String buffer = "[";

        buffer.reserve(JSON_BUFFER_SIZE);

        JsonObject obj = doc.to<JsonObject>();

        obj["if"] = "STATION";
        obj["enabled"] = (bool)(WiFi.getMode() & WIFI_STA);
        obj["connected"] = (bool)(WiFi.status() == WL_CONNECTED);
        obj["ssid"] = WiFi.SSID();
        obj["bssid"] = WiFi.BSSIDstr();
        obj["mac"] = WiFi.macAddress();
        obj["rssi"] = WiFi.RSSI();
        obj["ip"] = WiFi.localIP().toString();

        serializeJson(doc, buffer);

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

        serializeJson(doc, buffer);

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
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        JsonObject obj = doc.to<JsonObject>();
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
        JsonObject obj2 = obj.createNestedObject("fsInfo");
        obj2["totalBytes"] = fsInfo.totalBytes;
        obj2["usedBytes"] = fsInfo.usedBytes;
        obj2["blockSize"] = fsInfo.blockSize;
        obj2["pageSize"] = fsInfo.pageSize;
        obj2["maxOpenFiles"] = fsInfo.maxOpenFiles;
        obj2["maxPathLength"] = fsInfo.maxPathLength;

        serializeJson(doc, buffer);
        WebServer.send(200, jsonContent, buffer);
    } else {
        sendNotAllowed();
    }
}

void handleLogger() {
    Logger.debug("handleLogger()");
    WebServer.debug();

    if (WebServer.method() == HTTP_POST) {
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        DeserializationError err = deserializeJson(doc, WebServer.arg("plain"));

        if (!err) {
            uint16_t port = doc["tcpPort"];
            if (port != 0) {
                Logger.debugf("Setting tcp port: %i", port);
                Logger.setTcpPort(port);
            }
            port = doc["udpPort"];
            if (port != 0) {
                Logger.debugf("Setting udp port: %i", port);
                Logger.setUdpPort(port);
            }
            String ipStr = doc["ip"];
            if (ipStr.length() != 0) {
                IPAddress ip;
                if (ip.fromString(ipStr)) {
                    Logger.debugf("Setting IP: %s", ipStr.c_str());
                    Logger.setIP(ip);
                }
            }
            if (doc.containsKey("tcp")) {
                if (doc["tcp"]) {
                    Logger.enableLog(LOG_TCP);
                } else {
                    Logger.disableLog(LOG_TCP);
                }
            }
            if (doc.containsKey("udp")) {
                if (doc["udp"]) {
                    Logger.enableLog(LOG_UDP);
                } else {
                    Logger.disableLog(LOG_UDP);
                }
            }
            if (doc.containsKey("uart")) {
                if (doc["uart"]) {
                    Logger.enableLog(LOG_UART);
                } else {
                    Logger.disableLog(LOG_UART);
                }
            }

            Logger.setLevel(doc["level"]);

            Logger.persist();

            WebServer.send(200, jsonContent, successBody);
        } else {
            sendNotAllowed();
        }
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

void handleSendFile() {
    Logger.debug("handleSendFile()");
    WebServer.debug();

    if (WebServer.method() == HTTP_GET && WebServer.hasArg("file")) {
        String fileName = WebServer.arg("file");
        if (SPIFFS.exists(fileName)) {
            File f = SPIFFS.open(fileName, "r");
            int ret = PicUpdater.sendFile(&f);
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

            sendDone();
        } else {
            sendNotAllowed();
        }
    } else {
        sendNotAllowed();
    }
}

void handleReadDeviceId() {
    uint16_t deviceId;
    uint16_t revisionId;
    char cStr[5];
    Logger.debug("handleReadDeviceId()");
    WebServer.debug();

    PicUpdater.getDeviceAndRevisionId(&deviceId, &revisionId);

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonObject obj = doc.to<JsonObject>();
    String buffer = "";
    buffer.reserve(JSON_BUFFER_SIZE);

    obj["deviceInt"] = deviceId;
    sprintf(cStr, "%04X", deviceId);
    obj["deviceId"] = String(cStr);

    obj["revisionInt"] = revisionId;
    sprintf(cStr, "%04X", revisionId);
    obj["revisionId"] = String(cStr);

    serializeJson(doc, buffer);
    WebServer.send(200, jsonContent, buffer);
}

void handleReadMemory() {
    uint16_t data[16];
    char buffer[16*4+1];
    Logger.debug("handleReadMemory()");
    WebServer.debug();

    if (WebServer.hasArg("addr") && WebServer.hasArg("count")) {
        int addr = WebServer.arg("addr").toInt();
        int count = WebServer.arg("count").toInt();

        if (count <= 16 && count > 0) {
            count = PicUpdater.readMemory(addr, count, data);
        }

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                sprintf(&buffer[i*4], "%04X", data[i]);
            }

            WebServer.send(200, textContent, String(buffer));
        } else {
            sendNotAllowed();
        }
    } else {
        sendNotAllowed();
    }
}

void handleSendCommand() {
    Logger.debug("handleSendCommand()");
    WebServer.debug();

    uint8_t ret;

    if (WebServer.hasArg("command")) {
        uint8_t command = WebServer.arg("command").toInt();

        if (WebServer.hasArg("value")) {
            uint16_t value = WebServer.arg("value").toInt();
            ret = UartComm.sendCommandWithValue(command, value);
        } else {
            ret = UartComm.sendCommand(command);
        }

        WebServer.send(200, textContent, String(ret));
    } else {
        sendNotAllowed();
    }
}

void handlePublish() {
    Logger.debug("handlePublish()");
    WebServer.debug();

    uint8_t ret = Mqtt.publishMessage("{\"Message\":\"Test Message\"}");

    WebServer.send(200, textContent, String(ret));
}

void handleDeviceInfo() {
    Logger.debug("handleDeviceInfo()");
    WebServer.debug();

    JsonObject obj;
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    String buffer = "";

    Logger.debug("At start");
    Device.debug();

    if (WebServer.method() == HTTP_POST) {
        Logger.debug("In post handling");
        DeserializationError err = deserializeJson(doc, WebServer.arg("plain"));

        if (!err) {
            Logger.debug("No error");
            obj = doc.as<JsonObject>();
            Device.fromJson(obj);
            Logger.debug("After update");
            Device.debug();
        } else {
            Logger.debug("Error");
            sendNotAllowed();
            return;
        }

        doc.clear();
        Logger.debug("After clear");
        Device.debug();
    } else {
        Logger.debug("Not Post");
    }

    obj = doc.to<JsonObject>();
    Device.toJson(obj);
    buffer.reserve(JSON_BUFFER_SIZE);

    serializeJson(obj, buffer);

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