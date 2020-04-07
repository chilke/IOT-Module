#ifndef UART_COMM_H
#define UART_COMM_H

#define UART_COMM_ACK 'K'
#define UART_COMM_NACK 'N'

#define UART_COMM_SUCCESS 0
#define UART_COMM_TIMEOUT 1
#define UART_COMM_NACK_ERROR 2
#define UART_COMM_DATA_ERROR 3

#define RESPONSE_TIMEOUT 1000

class IotUartComm {
public:
    IotUartComm();
    uint8_t sendDimmerValue(uint8_t channel, uint16_t value);

    void init();
private:
    uint8_t sendCommand(uint8_t command, uint16_t value);

    char buffer[5];
};

extern IotUartComm UartComm;

#endif