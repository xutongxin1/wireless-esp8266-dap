#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "main/wifi_configuration.h"
#include "main/bps_config.h"
#include "main/uart_config.h"

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
static const char *UART_TAG = "UART";
static void uart_setup(struct uart_configrantion *config);
void uart_rev(void *uartParameter)
{
    struct uart_configrantion config = *(struct uart_configrantion *)uartParameter;
    uart_port_t uart_num = config.uart_num;
    uart_setup(&config);
    char buffer[UART_BUF_SIZE];
    size_t uart_buf_len = 0;
    
    QueueHandle_t uart_queue = *config.uart_queue;
    while (1)
    {
        uart_events event;
        event.buff_len = 0;
        event.uart_send_buff = NULL;
        while (event.buff_len == 0)
        {
            uart_get_buffered_data_len(uart_num, &event.buff_len);
        }

        event.buff_len = event.buff_len > UART_BUF_SIZE ? UART_BUF_SIZE : event.buff_len;
        event.buff_len = uart_read_bytes(uart_num, event.uart_buffer, event.buff_len, pdMS_TO_TICKS(5));
        // buffer[uart_buf_len] = '\0';
        if (event.buff_len != 0)
        {
            // strncpy(event.uart_buffer, buffer, uart_buf_len);
            // event.buff_len = uart_buf_len;
            // uart_buf_len = 0;
            if(xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)
            {
            }
            else
            {
                ESP_LOGE(UART_TAG, "SEND TO QUEUE FAILD\n");
            }
            
        }
    }
    vTaskDelete(NULL);
}
void uart_send(void *uartParameter)
{
    struct uart_configrantion config = *(struct uart_configrantion *)uartParameter;
    uart_port_t uart_num = config.uart_num;
    uart_setup(&config);
    ESP_LOGE(UART_TAG, "config.uart_queue = %p  \n*config.uart_queue = %p\n", config.uart_queue, *config.uart_queue);
    QueueHandle_t uart_queue = *config.uart_queue;
    while (1)
    {
        uart_events event;
        int ret = xQueueReceive(uart_queue, &event, portMAX_DELAY);
        if (ret == pdTRUE)
        {
            uart_write_bytes(uart_num, (const char *)event.uart_send_buff, event.buff_len);
        }
    }
    vTaskDelete(NULL);
}
static void uart_setup(struct uart_configrantion *config)
{
    ESP_LOGE(UART_TAG, "pin.ch = %d  pin.mode = %d,\n", config->pin.CH, config->pin.MODE);
    if (config->pin.MODE == TX)
    {
        uart_set_pin(config->uart_num, config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        ESP_LOGE(UART_TAG, "config->uart_num = %d\n", config->uart_num);
        uart_param_config(config->uart_num, &config->uart_config);
        uart_driver_install(config->uart_num, 136, UART_BUF_SIZE, 0, NULL, 0);
        ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
    }
    else
    {
        uart_set_pin(config->uart_num, UART_PIN_NO_CHANGE, config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        ESP_LOGE(UART_TAG, "config->uart_num = %d\n", config->uart_num);
        uart_param_config(config->uart_num, &config->uart_config);
        uart_driver_install(config->uart_num, UART_BUF_SIZE, 0, 0, NULL, 0);
        ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
    }
}
