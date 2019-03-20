#include <IotLogger.h>

#include <IotTime.h>

IotTime::IotTime()
:milliRolls(0),
lastMillis(0),
sysToEpochOffset(0),
nextEpochSecondHold(0xFFFFFFFF),
nextSync(0),
curHost(0),
curDrift(0)
{
    udp.begin(12345);
}

void IotTime::handle() {
    uint32_t m = millis();

    if (m < lastMillis) {
        uint32_t state = xt_rsil(15); // Disable interrupts

        ++milliRolls;
        lastMillis = m;

        xt_wsr_ps(state); // Restore original state
    } else {
        lastMillis = m;
    }

    if (m >= nextSync) {
        uint32_t curEpochOffset = doNtpRequest(m);
        if (curEpochOffset) {
            Logger.debugf("CurEpochOffset: %u", curEpochOffset);
            if (!sysToEpochOffset) {
                sysToEpochOffset = curEpochOffset;
            }
            if (curEpochOffset > sysToEpochOffset) {
                curDrift++;
            } else if (curEpochOffset < sysToEpochOffset) {
                curDrift--;
            }
        }

        curHost++;

        if (curHost >= 3) {
            Logger.debugf("Adjusting offset by %i", curDrift);
            curHost = 0;
            if (curDrift > 0) {
                sysToEpochOffset++;
            } else if (curDrift < 0) {
                nextEpochSecondHold = sysSeconds(NULL)+5;
            }
            curDrift = 0;
        }

        nextSync = m+NTP_SYNC_DELAY;
    }
}

int IotTime::curTimeToBuffer(char *buf, int size) {
    uint16_t curMillis;
    uint32_t seconds = epochSeconds(sysSeconds(&curMillis));

    return toBuffer(seconds, curMillis, buf, size);
}

uint32_t IotTime::doNtpRequest(uint32_t m) {
    sntp_msg msg;
    String host;
    uint8_t count= 0;

    memset(&msg, 0, sizeof(msg));
    msg.li_vn_mode = 0b00100011;
    msg.transmit_timestamp[0] = m;

    if (curHost == 0) {
        host = NTP_HOST_0;
    } else if (curHost == 1) {
        host = NTP_HOST_1;
    } else {
        host = NTP_HOST_2;
    }

    Logger.debugf("Sending ntp request to %s", host.c_str());
//    Logger.debugf("Transmit Timestamp %u", msg.transmit_timestamp[0]);
 //   Logger.debug("Request packet:");
//    Logger.dump((uint8_t *)&msg, sizeof(msg));

#ifdef NTP_TESTING
    udp.beginPacket("192.168.101.15", 123+curHost);
#else
    udp.beginPacket(host.c_str(), 123);
#endif
    udp.write((uint8_t *)&msg, sizeof(msg));
    udp.endPacket();

    while (1) {
        while (!udp.parsePacket()) {
            if (++count > NTP_WAIT_LOOP) {
                Logger.debug("Ntp timeout");
                return 0;
            }
            delay(NTP_WAIT_DELAY);
        }
        if (udp.read((uint8_t *)&msg, sizeof(msg)) == sizeof(msg)) {
//            Logger.debug("Response packet:");
//            Logger.dump((uint8_t *)&msg, sizeof(msg));
            Logger.debugf("Resp Orig Timestamp %u", msg.originate_timestamp[0]);
            if (msg.originate_timestamp[0] == m) {
                break;
            } else {
                Logger.debug("Response originate_timestamp doesn't match");
            }
        } else {
            Logger.debug("Failed to read proper response size");
        }
    }

    Logger.debugf("Ntp response time: %u", count*NTP_WAIT_DELAY);

    if (msg.stratum != 0) {
        uint32_t ret = ntohl(msg.receive_timestamp[0]);
        Logger.debugf("Good response: %u", ret);
        return ret - NTP_TO_EPOCH - sysSeconds(NULL);
    } else {
        Logger.debug("Received KOD response");
    }

    return 0;
}

uint32_t IotTime::epochSeconds(uint32_t sysSeconds) {
    if (sysSeconds >= nextEpochSecondHold) {
        sysToEpochOffset--;
        nextEpochSecondHold = 0xFFFFFFFF;
    }
    return sysSeconds+sysToEpochOffset;
}

uint32_t IotTime::sysSeconds(uint16_t *curMillis) {
    /* Max uint32 is 4294967296.  This means 4294967 seconds per roll + 1 extra per 3
        Keep in mind msec rolls happen once per 50 days so we aren't losing or gaining much either way */
    uint32_t m = millis();
    uint32_t rolls = milliRolls + ((m < lastMillis) ? 1 : 0);
    uint32_t seconds = m/1000;

    if (rolls > 0) {
        seconds += rolls*4294967296;
        seconds += rolls/3;
    }

    if (curMillis) {
        *curMillis = (uint16_t)(m%1000);
    }

    return seconds;
}

IotTime Time;