#include <Arduino.h>

#include <IotModule.h>
#include <IotHexFileReader.h>


void IotHexFileReader::init(File *f) {
    this->f = f;
    f->seek(0);
    baseAddress = 0;
    bufPos = 0;
    bufSize = 0;
}

void IotHexFileReader::readMoreData() {
    uint8_t i = 0;
    while (bufPos < bufSize) {
        buffer[i] = buffer[bufPos];
        i++;
        bufPos++;
    }

    bufSize = f->readBytes(&buffer[i], sizeof(buffer)-i);
    bufSize += i;
    bufPos = 0;
}

int IotHexFileReader::getNextData(FileReaderData *data) {
    readMoreData();
    if (bufSize < 9) {
        return -1;
    }

    if (buffer[bufPos] != ':') {
        return HFR_INVALID_START;
    }

    bufPos++;

    uint32_t len = htoi32n(&buffer[bufPos], 2);
    if (len > sizeof(data->data) || (len & 1) != 0) {
        return HFR_INVALID_LENGTH;
    }
    bufPos+=2;
    uint32_t addr = htoi32n(&buffer[bufPos], 4);
    bufPos+=4;
    uint32_t type = htoi32n(&buffer[bufPos], 2);
    bufPos+=2;

    readMoreData();

    if (type == 1) {
        return HFR_DATA_DONE;
    } else if (type == 4) {
        baseAddress = htoi32n(&buffer[bufPos], 4);
        baseAddress <<= 16;
        bufPos+=7; //7 here to account for checksum and single line terminator
        if (buffer[bufPos] == 0x0A) {
            bufPos++;
        }
        return getNextData(data);
    } else if (type == 0) {
        data->address = baseAddress+addr;
        data->length = len;
        for (uint i = 0; i < len; i++) {
            if (bufSize-bufPos < 2) {
                readMoreData();
                if (bufSize-bufPos < 2) {
                    return -1;
                }
            }

            data->data[i] = htoi8n(&buffer[bufPos], 2);
            bufPos += 2;
        }
        readMoreData();
        bufPos+=3; //3 here to account for checksum and single line terminator
        if (buffer[bufPos] == 0x0A) {
            bufPos++;
        }
    } else {
        return HFR_UNSUPPORTED_RECORD;
    }

    char buffer[33];
    for (int i = 0; i < data->length; i++) {
        sprintf(&buffer[i*2], "%02X", data->data[i]);
    }
    Logger.debugf("getNextData %04X %02X", data->address, data->length);
    Logger.debug(buffer);

    return HFR_DATA_SUCCESS;
}

IotHexFileReader HexFileReader;