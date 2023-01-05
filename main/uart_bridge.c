/*
Copyright (c) 2015, SuperHouse Automation Pty Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file uart_bridge.c
 * @author windowsair
 * @brief UART TCP bridge
 * @version 0.1
 * @date 2021-11-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "main/wifi_configuration.h"
#include "main/bps_config.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netbuf.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include <lwip/netdb.h>

#include "cJSON.h"


#define EVENTS_QUEUE_SIZE 10


#ifdef CALLBACK_DEBUG
#define debug(s, ...) os_printf("%s: " s "\n", "Cb:", ##__VA_ARGS__)
#else
#define debug(s, ...)
#endif
static int uart_set_tcp(const char *message);
static int cJson_packaged(char *buffer, const char *data);
static const char *UART_TAG = "UART";
#define NETCONN_EVT_WIFI_DISCONNECTED (NETCONN_EVT_ERROR + 1)
//#define NETCONN_EVT_WIFI_DISCONNECTED -1
static QueueHandle_t uart_server_events_ch1 = NULL;
static QueueHandle_t uart_server_events_ch2 = NULL;
static QueueHandle_t uart_server_events_ch3 = NULL;
typedef struct
{
    struct netconn *nc;
    uint8_t type;
} netconn_events;

static uint8_t uart_read_buffer[UART_BUF_SIZE];
static char tcp_send_buffer[UART_BUF_SIZE];
// use lwip buffer to write back
static struct netconn *uart_netconn = NULL;
static bool is_conn_valid = false; // lock free
static bool is_first_time_recv = false;

static uint16_t choose_port(uint8_t pin)
{
    switch (pin)
    {
    case 25:
        return 1923;
    case 26:
        return 1922;
    case 34:
        return 1921;
    default:
        return -1;
    }
}
void uart_bridge_close(uint8_t ch)
{
    netconn_events events;
    events.type = NETCONN_EVT_WIFI_DISCONNECTED;
    switch (ch)
    {
    case 25:
        xQueueSend(uart_server_events_ch2, &events, 1000);
        break;
    case 26:
        xQueueSend(uart_server_events_ch3, &events, 1000);
        break;
    case 34:
        xQueueSend(uart_server_events_ch1, &events, 1000);
        break;
    default:
        break;
    }
    
}

static void uart_bridge_reset()
{
    uart_netconn = NULL;
    is_conn_valid = false;
}

/*
 * This function will be call in Lwip in each event on netconn
 */
static void netCallback_ch1(struct netconn *conn, enum netconn_evt evt, uint16_t length)
{
    // Show some callback information (debug)
    debug("sock:%u\tsta:%u\tevt:%u\tlen:%u\ttyp:%u\tfla:%02x\terr:%d",
          (uint32_t)conn, conn->state, evt, length, conn->type, conn->flags, conn->pending_err);

    netconn_events events;

    // If netconn got error, it is close or deleted, dont do treatments on it.
    if (conn->pending_err)
    {
        return;
    }
    // Treatments only on rcv events.
    switch (evt)
    {
    case NETCONN_EVT_RCVPLUS:
        events.nc = conn;
        events.type = evt;
        break;
    default:
        return;
        break;
    }

    // Send the event to the queue
    xQueueSend(uart_server_events_ch1, &events, pdMS_TO_TICKS(1000));
}
static void netCallback_ch2(struct netconn *conn, enum netconn_evt evt, uint16_t length)
{
    // Show some callback information (debug)
    debug("sock:%u\tsta:%u\tevt:%u\tlen:%u\ttyp:%u\tfla:%02x\terr:%d",
          (uint32_t)conn, conn->state, evt, length, conn->type, conn->flags, conn->pending_err);

    netconn_events events;

    // If netconn got error, it is close or deleted, dont do treatments on it.
    if (conn->pending_err)
    {
        return;
    }
    // Treatments only on rcv events.
    switch (evt)
    {
    case NETCONN_EVT_RCVPLUS:
        events.nc = conn;
        events.type = evt;
        break;
    default:
        return;
        break;
    }

    // Send the event to the queue
    xQueueSend(uart_server_events_ch2, &events, pdMS_TO_TICKS(1000));
}
static void netCallback_ch3(struct netconn *conn, enum netconn_evt evt, uint16_t length)
{
    // Show some callback information (debug)
    debug("sock:%u\tsta:%u\tevt:%u\tlen:%u\ttyp:%u\tfla:%02x\terr:%d",
          (uint32_t)conn, conn->state, evt, length, conn->type, conn->flags, conn->pending_err);

    netconn_events events;

    // If netconn got error, it is close or deleted, dont do treatments on it.
    if (conn->pending_err)
    {
        return;
    }
    // Treatments only on rcv events.
    switch (evt)
    {
    case NETCONN_EVT_RCVPLUS:
        events.nc = conn;
        events.type = evt;
        break;
    default:
        return;
        break;
    }

    // Send the event to the queue
    xQueueSend(uart_server_events_ch3, &events, pdMS_TO_TICKS(1000));
}

/*
 *  Initialize a server netconn and listen port
 */
static void set_tcp_server_netconn(struct netconn **nc, uint16_t port,uint8_t ch)
{
    uint8_t err = 111;
    #define MY_IP_ADDR(...) IP4_ADDR(__VA_ARGS__)
     tcpip_adapter_ip_info_t ip_info;
      MY_IP_ADDR(&ip_info.ip, UART_IP_ADDRESS);
    // MY_IP_ADDR(&ip, UART_IP_ADDRESS);
    if (nc == NULL)
    {
        ESP_LOGE(UART_TAG, "%s: netconn missing .\n", __FUNCTION__);
        return;
    }
    switch (ch)
    {
    case 25:
         *nc = netconn_new_with_callback(NETCONN_TCP, netCallback_ch2);
        break;
    case 26:
         *nc = netconn_new_with_callback(NETCONN_TCP, netCallback_ch3);
        break;
    case 34:
         *nc = netconn_new_with_callback(NETCONN_TCP, netCallback_ch1);
        break;
    default:
        break;
    }
   
    if (!*nc)
    {
        ESP_LOGE(UART_TAG, "Status monitor: Failed to allocate netconn.\n");
        return;
    }
    netconn_set_nonblocking(*nc, NETCONN_FLAG_NON_BLOCKING);
    // netconn_set_recvtimeout(*nc, 10);
    // netconn_bind(*nc, IP_ADDR_ANY, port);
    err = netconn_bind(*nc, &ip_info.ip, port);

    ESP_LOGE(UART_TAG, "netconn_bind is %d\n",err);
    err = netconn_listen(*nc);
    ESP_LOGE(UART_TAG, "netconn_listen is %d\n",err);

}

/*
 *  Close and delete a socket properly
 */
static void close_tcp_netconn(struct netconn *nc)
{
    nc->pending_err = ERR_CLSD; // It is hacky way to be sure than callback will don't do treatment on a netconn closed and deleted
    netconn_close(nc);
    netconn_delete(nc);
}

static void uart_bridge_setup(struct uart_configrantion* config)
{    
    ESP_LOGE(UART_TAG, "pin.ch = %d  pin.mode = %d,\n",config->pin.CH,config->pin.MODE);
    if (config->pin.MODE)
    {
        uart_set_pin(config->uart_num, config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        ESP_LOGE(UART_TAG, "config->uart_num = %d\n",config->uart_num);
        uart_param_config(config->uart_num, &config->uart_config);
        uart_driver_install(config->uart_num, 1, UART_BUF_SIZE, 0, NULL, 0); 
        ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
    }
    else
    {
        uart_set_pin(config->uart_num, UART_PIN_NO_CHANGE , config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        ESP_LOGE(UART_TAG, "config->uart_num = %d\n",config->uart_num);
        uart_param_config(config->uart_num, &config->uart_config);
        uart_driver_install(config->uart_num, UART_BUF_SIZE, 0, 0, NULL, 0); 
        ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
    }  
   
    
}
static void CreateUartQueue(uint8_t ch)
{
     switch (ch)
    {
    case 25:
        uart_server_events_ch2 = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(netconn_events));
        break;
    case 26:
        uart_server_events_ch3 = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(netconn_events));
        break;
    case 34:
        uart_server_events_ch1 = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(netconn_events));
        break;
    default:
        break;
    }
}
static int ReceiveUartQueue(uint8_t ch,netconn_events* events)
{
    switch (ch)
    {
    case 25:
        return  xQueueReceive(uart_server_events_ch2, events, pdMS_TO_TICKS(100));//从uart_server_events队列中获取一个事件
        break;
    case 26:
        return  xQueueReceive(uart_server_events_ch3, events, pdMS_TO_TICKS(100));//从uart_server_events队列中获取一个事件
        break;
    case 34:
        return  xQueueReceive(uart_server_events_ch1, events, pdMS_TO_TICKS(100));//从uart_server_events队列中获取一个事件
        break;
    default:
    return -1;
        break;
    } 
    return -1;
}
// void uart_bridge_init()
// {
//     uart_server_events = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(netconn_events));
// }

void uart_bridge_task(void* uartParameter)
{
    err_t error = 1;
    struct uart_configrantion config =*(struct uart_configrantion*)uartParameter;
    uart_port_t uart_num = config.uart_num;
    uint16_t ch_port = choose_port(config.pin.CH);
    ESP_LOGE(UART_TAG, "tcp port = %d\n",ch_port);
    uart_bridge_setup(&config);
    //创建事件队列
    CreateUartQueue(config.pin.CH);

    struct netconn *nc = NULL; // 创建服务端
    set_tcp_server_netconn(&nc, ch_port, config.pin.CH);// 创建服务端

    struct netbuf *netbuf = NULL; // 存放tcp接受到的数据
    struct netconn *nc_in = NULL; // 接受来到的连接请求 To accept incoming netconn

    char *buffer;
    uint16_t len_buf;
    size_t uart_buf_len;

    // while (error != ERR_OK)
    // {
    //     error = netconn_accept(nc, &nc_in);
    //     os_printf("accepted new connection %p\nerror = %d", nc,error);
    // }
    // is_conn_valid = true;
        ESP_LOGE(UART_TAG, "start while");
    while (1)
    {
        netconn_events events;//创建tcp事件
        int ret = ReceiveUartQueue(config.pin.CH,&events);
        

        // ESP_LOGE(UART_TAG, "events.type = %d\n",events.type);
        //ESP_LOGE(UART_TAG, "events.nc->state = %d\n",events.nc->state == NETCONN_LISTEN ? 1 : 0);
        if (ret != pdTRUE)
        {
            // timeout
            if (is_conn_valid && config.pin.MODE == RX)//如果连接成功
            {
                ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, &uart_buf_len));
                uart_buf_len = uart_buf_len > UART_BUF_SIZE ? UART_BUF_SIZE : uart_buf_len;
                uart_buf_len = uart_read_bytes(uart_num, uart_read_buffer, uart_buf_len, pdMS_TO_TICKS(5));
                // then send data //发送数据
                if (uart_buf_len != 0)
                {

                    //cJson_packaged(tcp_send_buffer, (const char *)uart_read_buffer);
                    //buffer清空
                    //memset(uart_read_buffer, 0, sizeof(uart_read_buffer));
                    // then send data
                    //转发数据
                    ESP_LOGE(UART_TAG, "%s",uart_read_buffer);
                    netconn_write(events.nc, uart_read_buffer, uart_buf_len, NETCONN_COPY);
                    //netconn_write(uart_netconn, tcp_send_buffer, strlen(tcp_send_buffer), NETCONN_COPY);
                    //memset(tcp_send_buffer, 0, sizeof(uart_read_buffer));
                    ESP_LOGE(UART_TAG, "send in ret != pdTRUE ch = %d\nCH_PORT = %d",config.pin.CH,ch_port);
                }
            }
        }
        else if (events.type == NETCONN_EVT_WIFI_DISCONNECTED)
        { // WIFI disconnected
            if (is_conn_valid)
            {
                ESP_LOGE(UART_TAG, "WIFI disconnected\nCH_PORT = %d",ch_port);
                close_tcp_netconn(uart_netconn);
                uart_bridge_reset();
            }
        }
        else if (events.nc->state == NETCONN_LISTEN)
        {
            ESP_LOGE(UART_TAG, "wait\nCH_PORT = %d",ch_port);
            // if (is_conn_valid)
            // {
            //     netconn_accept(events.nc, &nc_in);
            //     if (nc_in)
            //     {
            //         ESP_LOGE(UART_TAG, "close_tcp_netconn\n");
            //         close_tcp_netconn(nc_in);
            //         //is_conn_valid = false;
            //     }
            //     continue;
            // }

            int err = netconn_accept(events.nc, &nc_in);
            if (err != ERR_OK)
            {
                if (nc_in)
                {
                    ESP_LOGE(UART_TAG, "netconn_delete\nCH_PORT = %d",ch_port);
                    close_tcp_netconn(nc_in);
                    //netconn_delete(nc_in);
                    is_conn_valid = false;
                }
                continue;
            }

            ip_addr_t client_addr; // Address port
            uint16_t client_port;  // Client port
            netconn_peer(nc_in, &client_addr, &client_port);
            ESP_LOGE(UART_TAG, "netconn_peer\n");
            uart_netconn = nc_in;
            is_conn_valid = true;
        }
        else if (events.nc->state != NETCONN_LISTEN && config.pin.MODE == RX)
        {

            uart_netconn = events.nc;
            // read data from UART
            ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, &uart_buf_len));
            uart_buf_len = uart_buf_len > UART_BUF_SIZE ? UART_BUF_SIZE : uart_buf_len;
            uart_buf_len = uart_read_bytes(uart_num, uart_read_buffer, uart_buf_len, pdMS_TO_TICKS(5));
            if (uart_buf_len != 0)
            {
                //cJson_packaged(tcp_send_buffer, (const char *)uart_read_buffer);
               // memset(uart_read_buffer, 0, sizeof(uart_read_buffer));
                // then send data
                netconn_write(events.nc, uart_read_buffer, uart_buf_len, NETCONN_COPY);
                //netconn_write(events.nc, tcp_send_buffer, strlen(tcp_send_buffer), NETCONN_COPY);
               // memset(tcp_send_buffer, 0, sizeof(uart_read_buffer));
                ESP_LOGE(UART_TAG, "send in state != NETCONN_LISTEN\nch = %d\nCH_PORT = %d",config.pin.CH,ch_port);
            }

            // try to get data
            if ((netconn_recv(events.nc, &netbuf)) == ERR_OK && config.pin.MODE == TX) // data incoming ?
            {
                do
                {
                    netbuf_data(netbuf, (void *)&buffer, &len_buf);
                   
                } while (netbuf_next(netbuf) >= 0);
                netbuf_delete(netbuf);
                ESP_LOGE(UART_TAG, "re \nch = %d\nCH_PORT = %d",config.pin.CH,ch_port);
            }
            else
            {
                if (events.nc->pending_err == ERR_CLSD)
                {
                    ESP_LOGE(UART_TAG, "events.nc->pending_err = %d\n",events.nc->pending_err);
                    continue; // The same hacky way to treat a closed connection
                }
                close_tcp_netconn(events.nc);
                uart_bridge_reset();
            }
        }
    }
}

// static int cJson_packaged(char *buffer, const char *data)
// {
//     cJSON *cjson_test = NULL;
//     char *str;
//     int len;
//     /* 创建一个JSON数据对象(链表头结点) */
//     cjson_test = cJSON_CreateObject();
//     /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
//     cJSON_AddStringToObject(cjson_test, "uart", data);
//     /* 打印JSON对象(整条链表)的所有数据 */
//     str = cJSON_PrintUnformatted(cjson_test);
//     strcpy(buffer, str);
//     free(str);
//     cJSON_Delete(cjson_test);
//     len = strlen(buffer);
//     buffer[len++] = '\r';
//     buffer[len++] = '\n';
//     buffer[len] = '\0';
//     return len;
// }

// static int uart_set_tcp(const char *message)
// {
//     cJSON *test = NULL;
//     cJSON *attach = NULL;
//     cJSON *passageway = NULL;
//     cJSON *mode = NULL;
//     cJSON *band = NULL;
//     cJSON *parity = NULL;
//     cJSON *data = NULL;
//     cJSON *stop = NULL;

//     /* 解析整段JSO数据 */
//     test = cJSON_Parse(message);
//     attach = cJSON_GetObjectItem(test, "attach");
//     if (attach == NULL)
//     {
//         os_printf("parse fail.\n");
//         cJSON_Delete(test);
//         return -1;
//     }
//     os_printf("success get parse\r\n");
//     /* 依次根据名称提取JSON数据（键值对） */
//     passageway = cJSON_GetObjectItem(attach, "c1");
//     if (passageway != NULL)
//     {
//         mode = cJSON_GetObjectItem(passageway, "mode");
//         band = cJSON_GetObjectItem(passageway, "band");
//         parity = cJSON_GetObjectItem(passageway, "parity");
//         data = cJSON_GetObjectItem(passageway, "data");
//         stop = cJSON_GetObjectItem(passageway, "stop");
//         if (strcmp(mode->valuestring,"Input"))
//         {
//             /* code */
//         }
        
        
//         os_printf("mode: %s\n", mode->valuestring);
//         os_printf("band:%d\n", band->valueint);
//         os_printf("parity:%s\n", parity->valuestring);
//         os_printf("data:%d\n", data->valueint);
//         os_printf("stop:%d\n", stop->valueint);
//     }
//     passageway = cJSON_GetObjectItem(attach, "c2");
//     if (passageway != NULL)
//     {
//         mode = cJSON_GetObjectItem(passageway, "mode");
//         band = cJSON_GetObjectItem(passageway, "band");
//         parity = cJSON_GetObjectItem(passageway, "parity");
//         data = cJSON_GetObjectItem(passageway, "data");
//         stop = cJSON_GetObjectItem(passageway, "stop");
//         os_printf("success get C2 item\r\n");

//         os_printf("mode: %s\n", mode->valuestring);
//         os_printf("band:%d\n", band->valueint);
//         os_printf("parity:%s\n", parity->valuestring);
//         os_printf("data:%d\n", data->valueint);
//         os_printf("stop:%d\n", stop->valueint);
//     }
//     passageway = cJSON_GetObjectItem(attach, "c3");
//     if (passageway != NULL)
//     {
//         mode = cJSON_GetObjectItem(passageway, "mode");
//         band = cJSON_GetObjectItem(passageway, "band");
//         parity = cJSON_GetObjectItem(passageway, "parity");
//         data = cJSON_GetObjectItem(passageway, "data");
//         stop = cJSON_GetObjectItem(passageway, "stop");
//         os_printf("success get C3 item\r\n");

//         os_printf("mode: %s\n", mode->valuestring);
//         os_printf("band:%d\n", band->valueint);
//         os_printf("parity:%s\n", parity->valuestring);
//         os_printf("data:%d\n", data->valueint);
//         os_printf("stop:%d\n", stop->valueint);
//     }

//     cJSON_Delete(test);
//     return 0;
// }

// int main(void)
// {
//     cJSON* cjson_test = NULL;
//     cJSON* cjson_name = NULL;
//     cJSON* cjson_age = NULL;
//     cJSON* cjson_weight = NULL;
//     cJSON* cjson_address = NULL;
//     cJSON* cjson_address_country = NULL;
//     cJSON* cjson_address_zipcode = NULL;
//     cJSON* cjson_skill = NULL;
//     cJSON* cjson_student = NULL;
//     int    skill_array_size = 0, i = 0;
//     cJSON* cjson_skill_item = NULL;

//     /* 解析整段JSO数据 */
//     cjson_test = cJSON_Parse(message);
//     if(cjson_test == NULL)
//     {
//         printf("parse fail.\n");
//         return -1;
//     }

//     /* 依次根据名称提取JSON数据（键值对） */
//     cjson_name = cJSON_GetObjectItem(cjson_test, "name");
//     cjson_age = cJSON_GetObjectItem(cjson_test, "age");
//     cjson_weight = cJSON_GetObjectItem(cjson_test, "weight");

//     printf("name: %s\n", cjson_name->valuestring);
//     printf("age:%d\n", cjson_age->valueint);
//     printf("weight:%.1f\n", cjson_weight->valuedouble);

//     /* 解析嵌套json数据 */
//     cjson_address = cJSON_GetObjectItem(cjson_test, "address");
//     cjson_address_country = cJSON_GetObjectItem(cjson_address, "country");
//     cjson_address_zipcode = cJSON_GetObjectItem(cjson_address, "zip-code");
//     printf("address-country:%s\naddress-zipcode:%d\n", cjson_address_country->valuestring, cjson_address_zipcode->valueint);

//     /* 解析数组 */
//     cjson_skill = cJSON_GetObjectItem(cjson_test, "skill");
//     skill_array_size = cJSON_GetArraySize(cjson_skill);
//     printf("skill:[");
//     for(i = 0; i < skill_array_size; i++)
//     {
//         cjson_skill_item = cJSON_GetArrayItem(cjson_skill, i);
//         printf("%s,", cjson_skill_item->valuestring);
//     }
//     printf("\b]\n");

//     /* 解析布尔型数据 */
//     cjson_student = cJSON_GetObjectItem(cjson_test, "student");
//     if(cjson_student->valueint == 0)
//     {
//         printf("student: false\n");
//     }
//     else
//     {
//         printf("student:error\n");
//     }

//     return 0;
// }

// (USE_UART_BRIDGE == 1)