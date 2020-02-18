#include <Arduino.h>

#include <IotModule.h>

int IotPicUpdater::init(File *f) {
    int ret = initFile(f);

    if (ret != PU_SUCCESS) {
        return ret;
    }

    ret = validateFile();

    if (ret != PU_SUCCESS) {
        return ret;
    }

    ret = initFile(f);

    if (ret != PU_SUCCESS) {
        return ret;
    }

    enterProgramMode();

    return PU_SUCCESS;
}

int IotPicUpdater::initFile(File *f) {
    HexFileReader.init(f);

    if (HexFileReader.getNextData(&curFileData) != HFR_DATA_SUCCESS) {
        done = 1;
        return PU_NEXT_DATA_FAIL;
    }

    if (curFileData.address != 0) {
        done = 1;
        return PU_INV_START_ADDR;
    }

    curDataPos = 0;
    curAddr = 0;
    curRow = 0;

    // Should check the return here, but because we know the addr is 0, this can never fail
    updateMemoryMap();

    done = 0;
    validating = 0;
    resetPC = 1;

    return PU_SUCCESS;
}

int IotPicUpdater::sendFile() {
    int ret = 0;

    while (ret == 0) {
        ret = sendNextRow();
    }
    return ret;
}

void IotPicUpdater::getDeviceAndRevisionId(uint16_t *deviceId, uint16_t *revisionId) {
    resetPC = 1;
    curAddr = 0x8005;
    *revisionId = readNVM(true);
    *deviceId = readNVM(false);
}

int IotPicUpdater::validateFile() {
    int ret;
    validating = 1;
    ret = sendFile();
    validating = 0;
    return ret;
}

void IotPicUpdater::enterProgramMode() {
    pinMode(ICSP_CLK_PIN, OUTPUT);
    pinMode(ICSP_DAT_PIN, OUTPUT);
    digitalWrite(ICSP_CLK_PIN, 0);
    digitalWrite(ICSP_DAT_PIN, 0);
    digitalWrite(ICSP_MCLR_PIN, 0);
    delayMicroseconds(250);
    sendByte('M');
    sendByte('C');
    sendByte('H');
    sendByte('P');
}

void IotPicUpdater::exitProgramMode() {
    digitalWrite(ICSP_MCLR_PIN, 1);
}

int IotPicUpdater::sendNextRow() {
    if (done) {
        return done;
    }
    while ((curAddr & rowMask) == curRow) {
        uint16_t curVal = curFileData.data[curDataPos];
        curDataPos++;
        curVal |= (curFileData.data[curDataPos]<<8);
        curDataPos++;
        if (!sendNVM(curVal)) {
            done = 1;
            return PU_SEND_NVM_FAIL;
        }

        if (curDataPos >= curFileData.length) {
            int ret = HexFileReader.getNextData(&curFileData);
            if (ret == HFR_DATA_DONE) {
                done = 1;
                break;
            }
            if (ret != HFR_DATA_SUCCESS) {
                done = 1;
                return PU_NEXT_DATA_FAIL;
            }

            uint16_t newAddr = curFileData.length >> 1;
            if (newAddr <= curAddr) {
                done = 1;
                return PU_INVALID_ADDR;
            }
            if (newAddr != (curAddr+1)) {
                resetPC = 1;
            }
            curAddr = newAddr;
            if (curAddr > blockEnd) {
                updateMemoryMap();
            }
        }
    }

    //send commit row to PIC
    curRow = curAddr & rowMask;

    return done;
}

void IotPicUpdater::sendByte(uint8_t byte) {
    sendBytes((uint32_t)byte, 1);
}

void IotPicUpdater::sendValue(uint32_t value) {
    sendBytes(value << 1, 3);
}

void IotPicUpdater::sendBytes(uint32_t bytes, uint8_t count) {
    uint32_t mask = 0x00800000;

    if (count == 2) {
        mask = 0x00008000;
    } else if (count == 1) {
        mask = 0x00000080;
    }

    while (mask > 0) {
        digitalWrite(ICSP_CLK_PIN, 1);
        if ((bytes & mask) != 0) {
            digitalWrite(ICSP_DAT_PIN, 1);
        } else {
            digitalWrite(ICSP_DAT_PIN, 0);
        }
        digitalWrite(ICSP_CLK_PIN, 0);
        mask >>= 1;
    }
}

bool IotPicUpdater::updateMemoryMap() {
    bool ret = true;
    if (curAddr < 0x0800) {
        blockEnd = 0x07FF;
        rowMask = 0xFFE0;
    } else if (curAddr >= 0x8000 && curAddr < 0x8004) {
        blockEnd = 0x8003;
        rowMask = 0xFFFF;
    } else if (curAddr >= 0x8007 && curAddr < 0x800C) {
        blockEnd = 0x800B;
        rowMask = 0xFFFF;
    } else {
        ret = false;
    }

    return ret;
}

bool IotPicUpdater::sendNVM(uint16_t value) {
    //Update PC
    if (resetPC) {
        sendByte(PU_CMD_LOAD_PC);
        delayMicroseconds(1);
        sendValue(curAddr);
        delayMicroseconds(1);
        resetPC = 0;
    } else {
        sendByte(PU_CMD_INC_PC);
        delayMicroseconds(1);
    }

    sendByte(PU_CMD_LOAD_NVM);
    delayMicroseconds(1);
    sendValue(value);
    delayMicroseconds(1);

    uint16_t readValue = readNVM(false);

    return (readValue == value);
}

uint16_t IotPicUpdater::readNVM(bool inc) {
    uint16_t ret = 0;

    //Update PC
    if (resetPC) {
        sendByte(PU_CMD_LOAD_PC);
        delayMicroseconds(1);
        sendValue(curAddr);
        delayMicroseconds(1);
        resetPC = 0;
    }

    if (inc) {
        sendByte(PU_CMD_READ_NVM_INC);
    } else {
        sendByte(PU_CMD_READ_NVM);
    }

    pinMode(ICSP_DAT_PIN, INPUT_PULLUP);

    delayMicroseconds(1);

    for (int i = 0; i < 24; i++) {
        ret <<= 1;
        digitalWrite(ICSP_CLK_PIN, 1);
        digitalWrite(ICSP_CLK_PIN, 0);
        if (digitalRead(ICSP_DAT_PIN)) {
            ret |= 0x01;
        }
    }

    pinMode(ICSP_DAT_PIN, OUTPUT);

    delayMicroseconds(1);

    ret >>= 1;
    ret &= 0x3FFF;

    return ret;
}

IotPicUpdater PicUpdater;