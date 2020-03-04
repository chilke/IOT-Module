#include <ArduinoJson.h>
#include <FS.h>

#include <IotModule.h>
#include <IotLogger.h>
#include <WiFiCredentialManager.h>

WiFiCredentialManager::WiFiCredentialManager()
: _count(0)
{
}

void WiFiCredentialManager::load() {
    if (!SPIFFS.exists("/wifi_credentials.json")) {
        Logger.warn("/wifi_credentials.json does not exist");
        return;
    }
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    File f = SPIFFS.open("/wifi_credentials.json", "r");
    DeserializationError err = deserializeJson(doc, f);

    if (err) {
        Logger.error("WiFiCredentialManager::load() failed json parsing");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();

    _count = 0;
    for (JsonObject obj : arr) {
        credentials[_count].ssid = obj["ssid"].as<String>();
        credentials[_count++].password = obj["password"].as<String>();
    }
}

void WiFiCredentialManager::save() {
    Logger.debug("CredMan saving");
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonArray rootArr = doc.to<JsonArray>();

    for (int i = 0; i < _count; i++) {
        JsonObject obj = rootArr.createNestedObject();
        obj["ssid"] = credentials[i].ssid;
        obj["password"] = credentials[i].password;
    }

    File f = SPIFFS.open("/wifi_credentials.json", "w");
    serializeJson(doc, f);
    f.close();
}

void WiFiCredentialManager::add(String ssid, String password) {
    Logger.debugf("CredMan adding %s", ssid.c_str());
    for (int i = 0; i < _count; i++) {
        if (ssid.compareTo(credentials[i].ssid) == 0) {
            for (; i > 0; i--) {
                credentials[i].ssid = credentials[i-1].ssid;
                credentials[i].password = credentials[i-1].password;
            }
            credentials[0].ssid = ssid;
            credentials[0].password = password;

            return;
        }
    }
    for (int i = _count; i > 0; i--) {
        credentials[i].ssid = credentials[i-1].ssid;
        credentials[i].password = credentials[i-1].password;
    }

    credentials[0].ssid = ssid;
    credentials[0].password = password;

    if (_count < MAX_WIFI_CREDENTIALS) {
        _count++;
    }

    save();
}

bool WiFiCredentialManager::deleteSsid(String ssid) {
    Logger.debugf("CredMan deleting %s", ssid.c_str());
    for (int i = 0; i < _count; i++) {
        if (ssid.compareTo(credentials[i].ssid) == 0) {
            for (int j = i+1; j < _count; j++) {
                credentials[j-i].ssid = credentials[j].ssid;
                credentials[j-i].password = credentials[j].password;
            }

            _count--;
            save();
            return true;
        }
    }

    return false;
}

String WiFiCredentialManager::getSsid(int i) {
    return credentials[i].ssid;
}

String WiFiCredentialManager::getPassword(int i) {
    return credentials[i].password;
}

int WiFiCredentialManager::count() {
    return _count;
}