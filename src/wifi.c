#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/uart.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG = "wifi station";
static int s_retry_num = 0;
#define EXAMPLE_ESP_WIFI_SSID      "HUAWEI-AA1F2Y"
#define EXAMPLE_ESP_WIFI_PASS      "104205104205"

static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

struct WifiInfo
{
    unsigned char *ssid;
    unsigned char *password;
};

struct WifiInfo init_nvs(void)
{
    printf("[nvs] Startup..");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        // nvs_handle_t nvs_handle;
        // ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &nvs_handle));
        // ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "WIFI_SSID", "HUAWEI-AA1F2Y"));
        // ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "WIFI_PASSWORD", "104205104205"));
        // ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    }

    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &nvs_handle));
    // ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "WIFI_SSID", "HUAWEI-AA1F2Y"));
    // ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "WIFI_PASSWORD", "104205104205"));
    // ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    size_t nvs_size;
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "WIFI_SSID", NULL, &nvs_size));
    char *ssid = malloc(nvs_size);
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "WIFI_SSID", ssid, &nvs_size));

    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "WIFI_PASSWORD", NULL, &nvs_size));
    char *passwd = malloc(nvs_size);
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "WIFI_PASSWORD", passwd, &nvs_size));

    struct WifiInfo info = {(unsigned char*)ssid, (unsigned char*)passwd};
    nvs_close(nvs_handle);
    return info;
}

void init_wifi(void)
{
    struct WifiInfo info = init_nvs();
    uint8_t *ssid = info.ssid;
    uint8_t *password = info.password;
    wifi_sta_config_t conf = {
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .password = EXAMPLE_ESP_WIFI_PASS,
        .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    };
    wifi_config_t wifi_config = {
        .sta = conf,
        // .sta = {
        //     .ssid = {*ssid},
        //     .password = {*password},
        //     .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        //     .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        // },
    };
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
 
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 info.ssid, info.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 info.ssid, info.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}