# espnowSpeedTest

## My Configuration and Precautions

1. **Set target**. (My project is on ESP32-S2)
2. In **menuconfig**, set the CPU frequency to **maximum**.
3. In **menuconfig**, FreeRTOS -> configTICK_RATE_HZ -> 1000 (**optional**).
4. In **menuconfig**, turn **off** both WiFi AMPDU TX and AMPDU RX.
5. In **menuconfig**, **transmitting** side: WiFi Max number of WiFi dynamic **TX** buffers -> 48 (**optional**).
6. In **menuconfig**, **receiving** side: WiFi Max number of WiFi dynamic **RX** buffers -> 48 (**optional**).
7. Points to note in the code:
   1. After initializing WiFi, execute `esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS7_SGI);` to set the ESPNOW RF rate to the highest.
   2. `#define WIFI_CHANNEL 2` Choose a channel with less interference. You can use software to view the surrounding wireless environment (the simplest method is to scan for SSIDs to find an unused channel). Through testing, mine is channel 2. You can also try several channels; my tests show that **the choice of channel greatly affects the rate**.
   3. Since `esp_now_send();` is non-blocking, it is advisable to use a semaphore to control it to avoid errors like `ESP_ERR_ESPNOW_NO_MEM`(0x3067): Out of memory, as mentioned in the official documentation.

	> Note that too short interval between sending two ESP-NOW data may lead to disorder of sending callback function. So, it is recommended that sending the next ESP-NOW data after the sending callback function of the previous sending has returned.

```C
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(espnowsem, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken == pdTRUE)
        taskYIELD();
}

// other code
void sendTask(void *param) {
    uint8_t packet[100];
    memset(packet, 1, 100);
    espnowsem = xSemaphoreCreateBinary();
    xSemaphoreGive(espnowsem);
    while (true) {
        if(xSemaphoreTake(espnowsem, portMAX_DELAY) == pdTRUE)
        {
            esp_err_t result = esp_now_send(broadcast_mac_addr, packet, sizeof(packet));
            if (result != ESP_OK) ESP_LOGE(TAG, "Send Failed!%x",result);
        }
    }
}
// other code
```



### Result

```bash
I (140518) espnow: Frame rate: 1275.09 fps
I (144449) espnow: Frame rate: 1271.86 fps
I (148490) espnow: Frame rate: 1237.70 fps
I (152331) espnow: Frame rate: 1301.79 fps
I (156556) espnow: Frame rate: 1183.67 fps
I (160856) espnow: Frame rate: 1163.02 fps
I (165065) espnow: Frame rate: 1188.16 fps
I (169362) espnow: Frame rate: 1163.72 fps
I (173383) espnow: Frame rate: 1243.64 fps
```

