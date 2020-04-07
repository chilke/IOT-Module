#include <Arduino.h>

#include <IotModule.h>
#include <IotUartComm.h>

IotUartComm::IotUartComm() {
    Serial.setTimeout(5);
}

void IotUartComm::init() {
    Serial.flush();
    Serial.pins(15, 13);
    delay(1);
}

uint8_t IotUartComm::sendDimmerValue(uint8_t channel, uint16_t value) {
    uint8_t ret = sendCommand(channel, value);
    if (ret != UART_COMM_SUCCESS) {
        Logger.debug("sendCommand Failed, trying again");
        ret = sendCommand(channel, value);
    }

    return ret;
}

uint8_t IotUartComm::sendCommand(uint8_t command, uint16_t value) {
    uint8_t ret;
    
    while (Serial.available()) {
        Serial.read();
    }

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

    return ret;
}

IotUartComm UartComm;