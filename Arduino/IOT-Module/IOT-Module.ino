#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <FS.h>

#include <IotModule.h>

#define SAFE_MODE_SSID "CSHIOT-SAFE"

bool safeMode = false;
bool apMode = false;
bool otaRunning = false;

void setup() {
    Logger.begin(LOG_LEVEL_DEBUG, LOG_UART, 115200);
    if (!SPIFFS.begin()) {
        Logger.error("SPIFFS begin failure!!");
    }

    {
        uint32_t buf[RESET_SAFEMODE_SIZE];
        uint32_t cmp[] = RESET_SAFEMODE_DATA;

        if (ESP.rtcUserMemoryRead(RESET_SAFEMODE_ADDR, buf, RESET_SAFEMODE_SIZE*4)) {
            int i = 0;
            for (; i < RESET_SAFEMODE_SIZE; i++) {
                if (buf[i] != cmp[i]) {
                    safeMode = true;
                }
                buf[i] = 0;
            }
            if (!safeMode) {
                Logger.debug("Safemode enabled by RTC data");
                safeMode = true;
                ESP.rtcUserMemoryWrite(RESET_SAFEMODE_ADDR, buf, RESET_SAFEMODE_SIZE*4);
            } else {
                safeMode = false;
            }
        }
    }

    WiFi.begin("0");
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(false);

    /*  Setup PINs  */
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    /*  Cycle LED to indicate startup  */
    digitalWrite(LED_PIN, LED_RED);
    delay(500);
    digitalWrite(LED_PIN, LED_GREEN);

    /*  Wait 3 seconds to allow time to press button */
    delay(3000);

    /*  Check for safemode startup  */
    int count = 0;
    if (digitalRead(BOOT_BUTTON_PIN) == 0) {
    /*  Flip LED to indicate we're in AP MODE  */
    /*  Holding button for another 3 seconds will go to SAFE MODE  */
        digitalWrite(LED_PIN, LED_RED);
        apMode = true;
        while (digitalRead(BOOT_BUTTON_PIN) == 0) {
            Logger.debugf("Safe Boot Pin Low: %d", count);
            if (++count > 30) {
                safeMode = true;
            }
            delay(100);
        }
    }

    if (safeMode) {
        /*  Start AP directly through ESP8266WiFi  */
        WiFi.enableSTA(false);
        /*  Taken from library, set local IP to 192.168.64.1  */
        WiFi.softAPConfig(0x0140A8C0, 0x0140A8C0, 0x00FFFFFF);
        WiFi.softAP(SAFE_MODE_SSID);
    } else {
        WCM.begin(apMode);
        WebServer.begin();
    }

    otaSetup();
}

void loop() {
    /*  Handle connection manager events  */
    if (!safeMode) {
        WCM.handle();
        WebServer.handleClient();
        Time.handle();
    }

    ArduinoOTA.handle();
}

void otaSetup() {
    ArduinoOTA.onStart(otaStart);
    ArduinoOTA.onProgress(otaProgress);
    ArduinoOTA.onEnd(otaEnd);
    ArduinoOTA.onError(otaError);
    ArduinoOTA.begin();
}

void otaStart() {
    Logger.info("OTA Starting Update");
    otaRunning = true;
    /* Need to add logic to disable other processing here */
}

void otaProgress(unsigned int progress, unsigned int total) {
    Logger.debugf("OTA Progress: %u/%u - %u%%", progress, total, (progress / (total / 100)));
}

void otaEnd() {
    otaRunning = false;
}

void otaError(ota_error_t error) {
    switch (error) {
        case OTA_AUTH_ERROR:
        Logger.error("OTA Auth Failed");
        break;
        case OTA_BEGIN_ERROR:
        Logger.error("OTA Begin Failed");
        break;
        case OTA_CONNECT_ERROR:
        Logger.error("OTA Connect Failed");
        break;
        case OTA_RECEIVE_ERROR:
        Logger.error("OTA Receive Failed");
        break;
        case OTA_END_ERROR:
        Logger.error("OTA End Failed");
        break;
    }
    otaRunning = false;
}
