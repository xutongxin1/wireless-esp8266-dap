#ifndef __TCP_SERVER1_H__
#define __TCP_SERVER1_H__

#define PORT                        1920
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

typedef struct
{
   int mode;
   char* band;
   char* parity;
   int data;
   int stop;

}Uart_parameter_Analysis;



void tcp_server_task1(void *pvParameters);
void Heart_beat(unsigned int len,char *rx_buffer);
void commandJsonAnalysis(unsigned int len, void *rx_buffer,int ksock);
void attach_status(char str_attach);
void nvs_flash_write(char modeNumber,int listen_sock);
void nvs_flash_read(int listen_sock);
int UartC1ParameterAnalysis(char *attachRxBuffer,Uart_parameter_Analysis *t);
int UartC2ParameterMode(char *attachRxBuffer,Uart_parameter_Analysis *t);
int UartC3ParameterAnalysis(char *attachRxBuffer,Uart_parameter_Analysis *t);
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