#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

void esp_print_tasks(void)
{
    char *pbuffer = (char *)calloc(1, 2048);
    printf("--------------- heap:%lu ---------------------\r\n", esp_get_free_heap_size());
    vTaskGetRunTimeStats(pbuffer);
    printf("%s", pbuffer);
    printf("----------------------------------------------\r\n");
    free(pbuffer);
}

void monitor_task()
{
    while(1) {
        esp_print_tasks();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}