#ifndef HEX_FILE_READER_H
#define HEX_FILE_READER_H

#include <FS.h>

#define HFR_BUF_SIZE 64

#define HFR_DATA_DONE 1
#define HFR_DATA_SUCCESS 0
#define HFR_INVALID_START -1
#define HFR_UNSUPPORTED_RECORD -2
#define HFR_INVALID_LENGTH -3

typedef struct frd {
    uint16_t length;
    uint32_t address;
    uint8_t data[32];
} FileReaderData;

class IotHexFileReader {
public:
    void init(File *f);
    int getNextData(FileReaderData *data);
private:
    File *f;
    char buffer[HFR_BUF_SIZE];
    uint32_t baseAddress;
    uint8_t bufPos;
    uint8_t bufSize;
    void readMoreData();
    uint32_t htoin(char *hex, int count);
};

extern IotHexFileReader HexFileReader;

#endif