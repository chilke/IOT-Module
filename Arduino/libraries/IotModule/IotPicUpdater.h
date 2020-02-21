#ifndef PIC_UPDATER_H
#define PIC_UPDATER_H

#include <Arduino.h>
#include <IotHexFileReader.h>

#define PU_DONE 1
#define PU_SUCCESS 0
#define PU_NEXT_DATA_FAIL -1
#define PU_INV_START_ADDR -2
#define PU_SEND_NVM_FAIL -3
#define PU_INVALID_ADDR -4

#define PU_CMD_LOAD_PC 0x80
#define PU_CMD_INC_PC 0xF8
#define PU_CMD_BULK_ERASE 0x18
#define PU_CMD_LOAD_NVM 0x00
#define PU_CMD_READ_NVM 0xFC
#define PU_CMD_READ_NVM_INC 0xFE
#define PU_CMD_COMMIT 0xE0

class IotPicUpdater {
public:
    IotPicUpdater();
    int validateFile(File *f);
    int sendNextRow();
    int sendFile(File *f);
    void getDeviceAndRevisionId(uint16_t *deviceId, uint16_t *revisionId);
    void enterProgramMode();
    void exitProgramMode();
    void bulkErase();
    uint8_t readMemory(uint16_t addr, uint8_t count, uint16_t *data);
private:
    int initFile(File *f);
    void sendByte(uint8_t byte);
    void sendValue(uint32_t value);
    void sendBytes(uint32_t bytes, uint8_t count);
    bool updateMemoryMap();
    void sendNVM(uint16_t value);
    uint16_t readNVM(bool inc);

    FileReaderData curFileData;
    uint16_t blockEnd;
    uint16_t rowMask;
    uint16_t curRow;
    uint16_t curAddr;
    uint8_t curDataPos;
    uint8_t resetPC;
    uint8_t validating;
};

extern IotPicUpdater PicUpdater;

#endif