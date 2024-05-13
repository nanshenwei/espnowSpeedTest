# espnowSpeedTest

## My Configuration and Precautions

1. **Set target**. (My project is on ESP32-S2)
2. In **menuconfig**, set the CPU frequency to **maximum**.
3. In **menuconfig**, FreeRTOS -> configTICK_RATE_HZ -> 1000 (**optional**).
   1. This is also optional because in the send callback function it calls: `if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();` and the sending task's priority is `configMAX_PRIORITIES`.
4. In **menuconfig**, turn **off** both WiFi AMPDU TX and AMPDU RX.
5. ~~In **menuconfig**, **transmitting** side: WiFi Max number of WiFi dynamic **TX** buffers -> 48 (**optional**).~~
6. ~~In **menuconfig**, **receiving** side: WiFi Max number of WiFi dynamic **RX** buffers -> 48 (**optional**).~~
7. ❗️❗️❗️(Update) The test was conducted in an office environment at a distance of half a meter. For harsh electromagnetic environments and slightly longer transmission distances, **reducing the wireless transmission rate can significantly improve the transmission success rate (frame rate)**, for example: `esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS7_LGI); -> esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS1_LGI);`
   1. Tests have shown that high transmission rates can trigger the ESPNOW send callback function at a very high speed, achieving a high **"send rate"**, but packet capture reveals that the vast majority of received packets are erroneous packets that fail the CRC check (and thus do not trigger the ESPNOW receive callback function), significantly reducing the transmission success rate (frame rate).
   2. Frame rate calculations are performed at the receiving end's ESPNOW receive callback function. Since ESPNOW complies with the 802.11 standards, packets that trigger the ESPNOW receive callback function are **highly likely** to be correct (of course, in scenarios where accuracy is crucial, combining reliable data verification methods and robust retransmission mechanisms is necessary), so I believe this frame rate can relatively accurately reflect the rate of successfully transmitted packets.
8. ❗️❗️❗️Points to note in the code:
   1. After initializing WiFi, execute `esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS7_SGI);` to set the ESPNOW RF rate to the highest. (❗️❗️❗️Not suitable for all scenarios, detail in step 7⬆️)
   2. `#define WIFI_CHANNEL 2` Choose a channel with less interference. You can use software to view the surrounding wireless environment (the simplest method is to scan for SSIDs to find an unused channel). Through testing, mine is channel 2. You can also try several channels; my tests show that **the choice of channel greatly affects the rate**.
   3. ❗️❗️❗️Since `esp_now_send();` is non-blocking, it is advisable to use a semaphore to control it to avoid errors like `ESP_ERR_ESPNOW_NO_MEM`(0x3067): Out of memory, as mentioned in the official documentation.

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

