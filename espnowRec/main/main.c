#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include <string.h>

#define WIFI_CHANNEL 2

static const char *TAG = "espnow";

static uint32_t frame_count = 0;
static int64_t start_time = 0;

// ESPNOW接收回调函数
void espnow_recv_cb(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len) {
        frame_count++;

        if (frame_count % 5000 == 0) {
            float frame_rate = 5000.0 / ((esp_timer_get_time() - start_time) * 0.000001);
            ESP_LOGI(TAG, "Frame rate: %.2f fps", frame_rate);
            start_time = esp_timer_get_time();
        }
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

// 初始化ESPNOW
void espnow_init() {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "ESPNOW initialization complete");
}

void app_main(void) {
    esp_err_t ret;
    // 初始化NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    // 初始化Wi-Fi和ESPNOW
    wifi_init();
    ret = esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS7_SGI);
    if (ret != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Config ESP-NOW rate failed: %s", esp_err_to_name(ret));
        return;
    }
    espnow_init();
}
