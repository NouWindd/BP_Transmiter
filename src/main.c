#include "esp_err.h"
#include "esp_system.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "joystick.h"
#include "display.h"
#include "utils.h"
#include "imu.h"
#include "ui_controller.h"
#include "EspNowRcLink/transmitter.h"

#define ADC_MAX_FLOAT    4095.0f

static const char *TAG = "MAIN";

transmitter_t my_tx;
SemaphoreHandle_t i2c_mutex = NULL;
int8_t g_rssi = -100;

void app_main(void) {
    i2c_mutex = xSemaphoreCreateMutex();

    joystick_init();
    utils_init_i2c_bus();
    imu_init();
    display_init();
    switch_init();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = transmitter_init(&my_tx, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize transmitter!");
        return;
    }

    ESP_LOGI(TAG, "Transmitter started. Searching for receiver...");
    
    bool button_was_clicked = false;
    int cycle_count = 0;

    while (1) {
        if (g_button_clicked) {
            button_was_clicked = true;
        }
        if (!g_is_armed) {
            button_was_clicked = false;
        }

        g_signal_strength = rssi > -100 ? (uint8_t)((rssi + 100) * 255 / 100) : 0;

        // Mix Channels
        uint16_t roll     = (uint16_t)((g_gyro_joy.virt_x / ADC_MAX_FLOAT) * 1000.0f + 1000.0f);
        uint16_t pitch    = (uint16_t)((g_gyro_joy.virt_y / ADC_MAX_FLOAT) * 1000.0f + 1000.0f);
        uint16_t throttle = (uint16_t)((g_joy.virt_x      / ADC_MAX_FLOAT) * 1000.0f + 1000.0f);
        uint16_t yaw      = (uint16_t)((g_joy.virt_y      / ADC_MAX_FLOAT) * 1000.0f + 1000.0f);
        
        transmitter_set_channel(&my_tx, 0, roll);
        transmitter_set_channel(&my_tx, 1, 3000 - pitch);
        transmitter_set_channel(&my_tx, 2, 3000 - yaw);
        transmitter_set_channel(&my_tx, 3, throttle);
        transmitter_set_channel(&my_tx, 4, (g_is_armed && !button_was_clicked) ? 2000 : 1000);

        transmitter_commit(&my_tx);
        transmitter_update(&my_tx);

        if (cycle_count % 10 == 0) {
            g_rssi = rssi; 

            ui_controller_update();
            display_update();
            cycle_count = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
        cycle_count++;
    }
}