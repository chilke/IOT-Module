#ifndef UART_COMM_H
#define UART_COMM_H

#define UART_COMM_ACK 'K'
#define UART_COMM_NACK 'N'

#define UART_COMM_SUCCESS 0
#define UART_COMM_TIMEOUT 1
#define UART_COMM_NACK_ERROR 2
#define UART_COMM_DATA_ERROR 3

class IotUartComm {
public:
    uint8_t sendCommand(uint8_t command);
    uint8_t sendCommandWithValue(uint8_t command, uint16_t value);

private:

    void begin();
    void end();

    char buffer[5];
};

extern IotUartComm UartComm;

#endif