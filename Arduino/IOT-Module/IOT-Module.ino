#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <FS.h>

#include <IotModule.h>

#define SAFE_MODE_SSID "CSHIOT-SAFE"

bool safeMode = false;
bool apMode = false;

void setup() {
    pinMode(ICSP_CLK_PIN, INPUT);
    pinMode(ICSP_DAT_PIN, INPUT);
    pinMode(ICSP_MCLR_PIN, OUTPUT);
    digitalWrite(ICSP_MCLR_PIN, 1);
    Serial.begin(115200);

    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS failure");
        while (1);
    }

    Logger.init();

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
        UartComm.init();
        Device.init();  //Device must be init before Scheduler :)
        Scheduler.init();
        WCM.begin(apMode);
        WebServer.begin();
        Mqtt.init();
    }
}

void loop() {
    /*  Handle connection manager events  */
    if (!safeMode) {
        WCM.handle();
        WebServer.handleClient();
        Device.handle();
        if (WiFi.status() == WL_CONNECTED) {
            if (Time.isSet()) {
                Mqtt.handle();
                Scheduler.handle();
            }
        }
    }
}
