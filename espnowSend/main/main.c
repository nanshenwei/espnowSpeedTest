#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <string.h>

#define WIFI_CHANNEL 2

static const char* TAG = "ESPNOW";
#define BLINK_GPIO 9

static uint8_t s_led_state = 0;
SemaphoreHandle_t espnowsem = NULL;
// ESPNOW数据结构体
typedef struct {
    uint8_t data[100];  // 数据包大小200字节
} espnow_packet_t;

// 广播地址
uint8_t broadcast_mac_addr[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ESPNOW发送回调
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(espnowsem, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken == pdTRUE)
        taskYIELD();
}

// 初始化Wi-Fi
void wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
}

// 初始化ESP-NOW
void espnow_init() {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    esp_now_peer_info_t peer_info = {
        .peer_addr = {0},
        .channel = 0,
        .encrypt = false,
    };
    memcpy(peer_info.peer_addr, broadcast_mac_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
}

void gpio_init() {
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

// 发送任务
void sendTask(void *param) {
    espnow_packet_t packet;
    memset(&packet, 1, sizeof(espnow_packet_t));
    espnowsem = xSemaphoreCreateBinary();
    xSemaphoreGive(espnowsem);
    ESP_LOGI(TAG, "Ready to send data");
    while (true) {
        // 发送数据包
        if(xSemaphoreTake(espnowsem, portMAX_DELAY) == pdTRUE)
        {
            esp_err_t result = esp_now_send(broadcast_mac_addr, (uint8_t *)&packet, sizeof(espnow_packet_t));
            if (result != ESP_OK) {
                ESP_LOGE(TAG, "Send Failed!%x",result);
            }
            // 翻转LED状态
            // s_led_state = !s_led_state;
            // gpio_set_level(BLINK_GPIO, s_led_state);
        }
    }
    
}


void app_main() {
    esp_err_t ret;
    gpio_init();
    nvs_flash_init();
    wifi_init();
    ret = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS7_SGI);
    if (ret != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Config ESP-NOW rate failed: %s", esp_err_to_name(ret));
        return;
    }
    espnow_init();

    // 创建任务
    xTaskCreate(
        sendTask,                  // 任务函数
        "sendTask",                // 任务名称
        2048,                      // 栈大小
        NULL,                      // 传递给任务的参数
        configMAX_PRIORITIES,      // 任务优先级，几乎最高优先级
        NULL                       // 任务句柄
    );

}