#include <Arduino.h>

#include <IotModule.h>

IotPicUpdater::IotPicUpdater() {
    resetPC = 1;
    curAddr = 0;
    validating = 0;
}

int IotPicUpdater::initFile(File *f) {
    HexFileReader.init(f);

    if (HexFileReader.getNextData(&curFileData) != HFR_DATA_SUCCESS) {
        return PU_NEXT_DATA_FAIL;
    }

    if (curFileData.address != 0) {
        return PU_INV_START_ADDR;
    }

    curDataPos = 0;
    curAddr = 0;
    curRow = 0;

    // Should check the return here, but because we know the addr is 0, this can never fail
    updateMemoryMap();

    resetPC = 1;

    return PU_SUCCESS;
}

int IotPicUpdater::sendFileData(File *f) {
    int ret;

    ret = initFile(f);

    if (ret != PU_SUCCESS) {
        return ret;
    }

    ret = 0;

    while (ret == 0) {
        ret = sendNextRow();
    }

    return ret;
}

int IotPicUpdater::sendFile(File *f) {
    int ret;
    validating = 1;
    ret = sendFileData(f);
    validating = 0;
    if (ret != PU_DONE) {
        return ret;
    }
    enterProgramMode();
    bulkErase();
    ret = sendFileData(f);
    if (ret != PU_DONE) {
        bulkErase();
        ret = sendFileData(f);
        if (ret != PU_DONE) {
            bulkErase();
        }
    }
    exitProgramMode();
    return ret;
}

void IotPicUpdater::getDeviceAndRevisionId(uint16_t *deviceId, uint16_t *revisionId) {
    enterProgramMode();
    resetPC = 1;
    curAddr = 0x8005;
    *revisionId = readNVM(true);
    *deviceId = readNVM(false);
    exitProgramMode();
}

void IotPicUpdater::enterProgramMode() {
    //Should never be a problem, but make sure serial isn't using our PINs
    Serial.pins(1,3);
    digitalWrite(ICSP_MCLR_PIN, 0);
    pinMode(ICSP_CLK_PIN, OUTPUT);
    pinMode(ICSP_DAT_PIN, OUTPUT);
    digitalWrite(ICSP_CLK_PIN, 0);
    digitalWrite(ICSP_DAT_PIN, 0);
    
    delayMicroseconds(250);
    sendByte('M');
    sendByte('C');
    sendByte('H');
    sendByte('P');
}

void IotPicUpdater::exitProgramMode() {
    pinMode(ICSP_CLK_PIN, INPUT);
    pinMode(ICSP_DAT_PIN, INPUT);
    digitalWrite(ICSP_MCLR_PIN, 1);
}

void IotPicUpdater::bulkErase() {
    //Set PC to 0x0000 to ensure we erase program flash and configuration words
    sendByte(PU_CMD_LOAD_PC);
    delayMicroseconds(1);
    sendValue(0x0000);
    delayMicroseconds(1);

    sendByte(PU_CMD_BULK_ERASE);
    delay(9);
}

uint8_t IotPicUpdater::readMemory(uint16_t addr, uint8_t count, uint16_t *data) {
    enterProgramMode();
    resetPC = 1;
    curAddr = addr;

    if (!updateMemoryMap()) {
        return 0;
    }

    if (curAddr + count > blockEnd) {
        count = blockEnd - curAddr + 1;
    }

    for (int i = 0; i < count; i++) {
        data[i] = readNVM(true);
        curAddr++;
    }

    exitProgramMode();

    return count;
}

int IotPicUpdater::sendNextRow() {
    uint16_t rowAddr[32];
    uint16_t rowData[32];
    uint8_t rowPos = 0;
    uint16_t newAddr;
    uint16_t curVal;
    int ret = HFR_DATA_SUCCESS;

    resetPC=1;
    while ((curAddr & rowMask) == curRow) {
        curVal = curFileData.data[curDataPos];
        curDataPos++;
        curVal |= (curFileData.data[curDataPos]<<8);
        curDataPos++;
        rowData[rowPos] = curVal;
        rowAddr[rowPos] = curAddr;
        rowPos++;
        // Logger.debugf("sendNVM %04X %04X", curAddr, curVal);
        sendNVM(curVal);

        curAddr++;

        if (curDataPos >= curFileData.length) {
            ret = HexFileReader.getNextData(&curFileData);
            if (ret == HFR_DATA_DONE) {
                break;
            }
            if (ret != HFR_DATA_SUCCESS) {
                return PU_NEXT_DATA_FAIL;
            }

            newAddr = curFileData.address >> 1;
            if (newAddr < curAddr) {
                return PU_INVALID_ADDR;
            }
            if (newAddr != curAddr) {
                resetPC = 1;
            }
            curAddr = newAddr;
            curDataPos = 0;
        }
    }

    if (!validating) {
        // Logger.debugf("Before delay %lu", micros());
        sendByte(PU_CMD_COMMIT);
        delay(6);
        // Logger.debugf("After delay %lu", micros());

        newAddr = curAddr;

        for (int i = 0; i < rowPos; i++) {
            // Logger.debugf("curAddr: %04X, rowAddr: %04X", curAddr, rowAddr[i]);
            if (curAddr != rowAddr[i]) {
                // Logger.debug("Don't match");
                curAddr = rowAddr[i];
                resetPC = 1;
            }
            curVal=readNVM(true);
            // Logger.debugf("Compare addr: %04X, %04X, %04X", curAddr, curVal, rowData[i]);
            if (curVal != rowData[i]) {
                return PU_SEND_NVM_FAIL;
            }
            curAddr++;
        }

        if (curAddr != newAddr) {
            curAddr = newAddr;
        }
    }

    if (curAddr > blockEnd) {
        updateMemoryMap();
    }

    curRow = curAddr & rowMask;

    return ret == HFR_DATA_DONE ? PU_DONE : PU_SUCCESS;
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

void IotPicUpdater::sendNVM(uint16_t value) {
    if (validating) {
        return;
    }
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
}

uint16_t IotPicUpdater::readNVM(bool inc) {
    uint16_t ret = 0;
    // Logger.debugf("Reading %04X", curAddr);
    //Update PC
    if (resetPC) {
        // Logger.debug("Reseting PC");
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