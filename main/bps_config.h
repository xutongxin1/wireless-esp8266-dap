#ifndef __BPS_CONFIG_H__
#define __BPS_CONFIG_H__
#include "driver/uart.h"
//TCP相关
#define TCP_BUF_SIZE 1024
#define QUEUE_BUF_SIZE 1024
enum CHIo
{
    CH1 = 34,
    CH2 = 25,
    CH3 = 26,

};
enum tcp_mode {
    SEND = 0,
    RECEIVE,
};
//UART_CONGFIG 串口配置参数

#define UART_BUF_SIZE 512
#define TCP_IP_ADDRESS 192, 168, 31, 228

//串口IO模式
enum UartIOMode {
    Closed = 0,//关闭
    Input,//输入,1通道可用
    Output,//输出,3通道可用
    SingleInput,//独占输入，1通道可用
    SingleOutput,//独占输出,3通道可用
    Follow1Output,//跟随1通道输出,2通道可用
    Follow3Input//跟随3通道输入,2通道可用
};

//串口引脚配置
#define RX 0
#define TX 1

struct uart_pin
{
    uint8_t CH;
    uint8_t MODE;
};

typedef struct uart_configrantion
{
    QueueHandle_t* uart_queue;
    struct uart_pin pin;
    uart_port_t uart_num;
    enum UartIOMode mode;
    uart_config_t uart_config;
};
typedef struct
{
    uint8_t uart_ch;
    char uart_buffer[QUEUE_BUF_SIZE];
    char* uart_send_buff;
    uint16_t buff_len;
} uart_events;
struct TcpUartParam
{
    QueueHandle_t* uart_queue;
    enum tcp_mode mode;
    uint8_t ch;
};

#endif