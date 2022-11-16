#ifndef __TCP_SERVER1_H__
#define __TCP_SERVER1_H__

#define PORT                        1920
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3


void tcp_server_task1(void *pvParameters);
void Heart_beat(unsigned int len,char *rx_buffer);
void Command_analysis(unsigned int len,void *rx_buffer);
void attach_status(char str_attach);

enum Command_mode {
    DAP=1,
    UART,
    ADC,
    DAC,
    PWM_Collect,
    PWM_Simulation,
    I2C,
    SPI,
    CAN
};

#endif