#include <Arduino.h>

#include <IotModule.h>
#include <IotUartComm.h>

void IotUartComm::begin() {
    Serial.flush();
    Serial.pins(15, 13);
    delay(1); //Hack to wait for garbled byte after swapping
    while (Serial.available()) {
        Serial.read();
    }
}

void IotUartComm::end() {
    while (Serial.available()) {
        Serial.read();
    }
    Serial.pins(1, 3);
}

uint8_t IotUartComm::sendCommand(uint8_t command) {
    uint8_t ret;
    Logger.debug("Entering sendCommand");
    begin();
    Serial.printf("S%02X0000", command);
    Serial.flush();
    ret = Serial.readBytes(buffer, 1);
    if (ret == 1) {
        if (buffer[0] == UART_COMM_ACK) {
            ret = UART_COMM_SUCCESS;
        } else if (buffer[0] == UART_COMM_NACK) {
            ret = UART_COMM_NACK_ERROR;
        } else {
            ret = UART_COMM_DATA_ERROR;
        }
    } else {
        ret = UART_COMM_TIMEOUT;
    }

    end();
    return ret;
}

uint8_t IotUartComm::sendCommandWithValue(uint8_t command, uint16_t value) {
    uint8_t ret;
    begin();

    Serial.printf("S%02X%04X", command, value);
    Serial.flush();
    ret = Serial.readBytes(buffer, 5);
    if (ret == 5) {
        if (buffer[4] == UART_COMM_ACK) {
            uint16_t retVal = htoi16n(buffer, 4);
            if (retVal != value) {
                ret = UART_COMM_DATA_ERROR;
            } else {
                ret = UART_COMM_SUCCESS;
            }
        } else if (buffer[4] == UART_COMM_NACK) {
            ret = UART_COMM_NACK_ERROR;
        } else {
            ret = UART_COMM_DATA_ERROR;
        }
    } else if (ret == 1 && buffer[0] == UART_COMM_NACK) {
        ret = UART_COMM_NACK_ERROR;
    } else if (ret == 0) {
        ret = UART_COMM_TIMEOUT;
    } else {
        ret = UART_COMM_DATA_ERROR;
    }

    end();
    return ret;
}

IotUartComm UartComm;