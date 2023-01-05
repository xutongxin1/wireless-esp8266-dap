
#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "main/wifi_configuration.h"
#include "main/bps_config.h"
#include "main/tcp.h"

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
#include "lwip/priv/api_msg.h"
#include <lwip/netdb.h>

#define EVENTS_QUEUE_SIZE 10

#ifdef CALLBACK_DEBUG
#define debug(s, ...) os_printf("%s: " s "\n", "Cb:", ##__VA_ARGS__)
#else
#define debug(s, ...)
#endif
#define MY_IP_ADDR(...) IP4_ADDR(__VA_ARGS__)
static const char *TCP_TAG = "TCP";
static err_t get_connect_state(struct netconn *conn);
static uint16_t choose_port(uint8_t pin)
{
    switch (pin)
    {
    case 25:
        return 1922;
    case 26:
        return 1923;
    case 34:
        return 1921;
    default:
        return -1;
    }
}

void tcp_send_server(void *Parameter)
{
    // os_printf("Parameter = %p  \n", Parameter);
    struct TcpUartParam *Param = (struct TcpUartParam *)Parameter;
    // os_printf("Param = %p  \n", Param);
    struct netconn *conn = NULL;
    // os_printf("conn\n");
    struct netconn *newconn = NULL;

    err_t err = 1;
    // uint16_t len = 0;
    // void *recv_data;
    // recv_data = (void *)pvPortMalloc(TCP_BUF_SIZE);
    // os_printf("Param->uart_queue = %p  \n*Param->uart_queue = %p\n", Param->uart_queue, *Param->uart_queue);
    QueueHandle_t uart_queue = *Param->uart_queue;
    tcpip_adapter_ip_info_t ip_info;
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    while (conn == NULL)
    {
        conn = netconn_new(NETCONN_TCP);
        os_printf("CONN: %p\n", conn);
    }
    // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
    netconn_bind(conn, &ip_info.ip, choose_port(Param->ch));

    /* Tell connection to go into listening mode. */
    netconn_listen(conn);
    os_printf("PORT: %d\nLISTENING.....\n", choose_port(Param->ch));
    /* Grab new connection. */
    while (1)
    {
        int re_err;
        err = netconn_accept(conn, &newconn);

        /* Process the new connection. */

        if (err == ERR_OK)
        {

            struct netbuf *buf;
            struct netbuf *buf2;
            void *data;
            while (1)
            {
                re_err = (netconn_recv(newconn, &buf));
                if (re_err == ERR_OK)
                {
                    do
                    {
                        uart_events event;
                        netbuf_data(buf, &data, &event.buff_len);
                        event.uart_send_buff = data;
                        if (xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)
                        {
                        }
                        else
                        {
                            ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                        }
                    } while ((netbuf_next(buf) >= 0));
                    netbuf_delete(buf2);
                    re_err = (netconn_recv(newconn, &buf2));
                    if (re_err == ERR_OK)
                    {
                        do
                        {
                            uart_events event;
                            netbuf_data(buf2, &data, &event.buff_len);
                            event.uart_send_buff = data;
                            if (xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)
                            {
                            }
                            else
                            {
                                ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                            }
                        } while ((netbuf_next(buf2) >= 0));
                        netbuf_delete(buf);
                    }
                }
                else if (re_err == ERR_CLSD)
                {
                    ESP_LOGE(TCP_TAG, "DISCONNECT PORT:%d\n", choose_port(Param->ch));
                    netbuf_delete(buf);
                    netbuf_delete(buf2);
                    break;
                }
            }
            netconn_close(newconn);
            netconn_delete(newconn);
            netconn_listen(conn);
        }
    }
    vTaskDelete(NULL);
}
void tcp_rev_server(void *Parameter)
{
    struct TcpUartParam *Param = (struct TcpUartParam *)Parameter;
    struct netconn *conn = NULL;
    struct netconn *newconn = NULL;
    // err_t re_err = 0;
    QueueHandle_t uart_queue = *Param->uart_queue;
    tcpip_adapter_ip_info_t ip_info;
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    while (conn == NULL)
    {
        conn = netconn_new(NETCONN_TCP);
        os_printf("CONN: %p\n", conn);
        conn->send_timeout = 0;
    }
    MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
    // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    netconn_bind(conn, &ip_info.ip, choose_port(Param->ch));
    /* Tell connection to go into listening mode. */
    netconn_listen(conn);
    os_printf("PORT: %d\nLISTENING.....\n", choose_port(Param->ch));
    /* Grab new connection. */
    while (1)
    {

        /* Process the new connection. */
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            while (1)
            {
                uart_events event;
                int ret = xQueueReceive(uart_queue, &event, portMAX_DELAY);
                if (ret == pdTRUE)
                {
                    netconn_write(newconn, event.uart_buffer, event.buff_len, NETCONN_NOCOPY);
                }
                if (get_connect_state(newconn) == ERR_CLSD)
                {
                    netconn_close(newconn);
                    netconn_delete(newconn);
                    netconn_listen(conn);
                    ESP_LOGE(TCP_TAG, "DISCONNECT PORT:%d\n", choose_port(Param->ch));
                    break;
                }
            }
        }
    }
    vTaskDelete(NULL);
}
static err_t get_connect_state(struct netconn *conn)
{
    void *msg;
    err_t err;
    if (sys_arch_mbox_tryfetch(&conn->recvmbox, &msg) != SYS_MBOX_EMPTY)
    {
        lwip_netconn_is_err_msg(msg, &err);
        if (err == ERR_CLSD)
        {
            return ERR_CLSD;
        }
    }

    return ERR_OK;
}