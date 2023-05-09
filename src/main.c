#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <driver/uart.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "wifi.h"


void app_main()
{
    printf("[APP] Startup..");
    printf("\n");
    printf("[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    printf("\n");
    printf("[APP] IDF version: %s", esp_get_idf_version());
    printf("\n");
    printf("================================================================\n");
    init_wifi();
    printf("================================================================\n");
}