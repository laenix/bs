#include <mqtt5_client.h>
#include <mqtt_client.h>


void init_mqtt(void){
    static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
        {"u", "laenix"},
        {"p", "Xubo1357924680"}
    };
    static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = "/bs",
    .correlation_data = "123456",
    .correlation_data_len = 6,
};
}