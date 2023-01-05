/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "main/tcp_server.h"
#include "main/tcp_netconn.h"
#include "main/kcp_server.h"
#include "main/bps_config.h"
#include "main/uart_config.h"
#include "main/tcp.h"

#include "main/timer.h"
#include "main/wifi_configuration.h"
#include "main/wifi_handle.h"

#include "components/corsacOTA/src/corsacOTA.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "mdns.h"

#define EVENTS_QUEUE_SIZE 10

extern void DAP_Setup(void);
extern void DAP_Thread(void *argument);
extern void SWO_Thread();

TaskHandle_t kDAPTaskHandle = NULL;

static const char *MDNS_TAG = "server_common";

void mdns_setup()
{
    // initialize mDNS
    int ret;
    ret = mdns_init();
    if (ret != ESP_OK)
    {
        ESP_LOGW(MDNS_TAG, "mDNS initialize failed:%d", ret);
        return;
    }

    // set mDNS hostname
    ret = mdns_hostname_set(MDNS_HOSTNAME);
    if (ret != ESP_OK)
    {
        ESP_LOGW(MDNS_TAG, "mDNS set hostname failed:%d", ret);
        return;
    }
    ESP_LOGI(MDNS_TAG, "mDNS hostname set to: [%s]", MDNS_HOSTNAME);

    // set default mDNS instance name
    ret = mdns_instance_name_set(MDNS_INSTANCE);
    if (ret != ESP_OK)
    {
        ESP_LOGW(MDNS_TAG, "mDNS set instance name failed:%d", ret);
        return;
    }
    ESP_LOGI(MDNS_TAG, "mDNS instance name set to: [%s]", MDNS_INSTANCE);
}

void app_main()
{

    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_init();
    // DAP_Setup();
    timer_init();

    static QueueHandle_t uart_queue1 = NULL;
    static QueueHandle_t uart_queue = NULL;
    uart_queue = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(uart_events));
    uart_queue1 = xQueueCreate(EVENTS_QUEUE_SIZE, sizeof(uart_events));
    static struct uart_configrantion uart_param = {
        .pin.CH = CH2,
        .pin.MODE = RX,
        .uart_config.baud_rate = 115200,
        .uart_config.data_bits = UART_DATA_8_BITS,
        .uart_config.parity = UART_PARITY_DISABLE,
        .uart_config.stop_bits = UART_STOP_BITS_1,
        .uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .mode = SingleOutput,
        .uart_num = UART_NUM_2,
    };
    uart_param.uart_queue = &uart_queue;
    static struct uart_configrantion uart_param1 = {
        .pin.CH = CH3,
        .pin.MODE = TX,
        .uart_config.baud_rate = 115200,
        .uart_config.data_bits = UART_DATA_8_BITS,
        .uart_config.parity = UART_PARITY_DISABLE,
        .uart_config.stop_bits = UART_STOP_BITS_1,
        .uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .mode = SingleOutput,
        .uart_num = UART_NUM_1,
    };
    uart_param1.uart_queue = &uart_queue1;
    static struct TcpUartParam tcp_param = {
        .mode = RECEIVE,
        .ch = CH2,
    };
    tcp_param.uart_queue = &uart_queue;
    static struct TcpUartParam tcp_param1 = {
        .mode = SEND,
        .ch = CH3,
    };
    tcp_param1.uart_queue = &uart_queue1;   
     xTaskCreate(uart_send, "uarts", 4096, (void *)&uart_param1, 10, NULL);
    os_printf("create uarts \n");
    os_printf("create tcps \n");
    xTaskCreate(tcp_send_server, "tcps", 4096, (void *)&tcp_param1, 10, NULL);
    os_printf("finish \n");
 
    os_printf("start create task \n");   
    xTaskCreate(uart_rev, "uartr", 3072, (void *)&uart_param, 10, NULL);
    os_printf("create uartr \n");
    xTaskCreate(tcp_rev_server, "tcpr", 4096, (void *)&tcp_param, 10, NULL);
    os_printf("create tcpr \n");

#if (USE_UART_BRIDGE == 1)

#endif
}
