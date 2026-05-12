//#include "utils.h"
#include "display.h"
#include "hw_config.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include "u8g2_esp32_hal.h"
#include <esp_timer.h>
#include <esp_log.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <esp_log.h>
#include "freertos/semphr.h"

static const char* TAG = "UTILS";

// ---Start of program logic---

void utils_init_i2c_bus(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);

    ESP_LOGI(TAG, "I2C bus initialized with SDA pin %d and SCL pin %d", I2C_SDA_PIN, I2C_SCL_PIN);
}

void get_i2c_devices(void)
{
    ESP_LOGI(TAG, "Scaning I2C Bus........");

    for (int addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);

        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device at address 0x%02x", addr);
        }

    }
    ESP_LOGI(TAG, "I2C scan completed");
}

void init_gpio_interrupt_service(void) {
    esp_err_t ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "GPIO ISR Service install failed!");
    }
}

void add_joystick_interrupt(gpio_num_t pin, gpio_isr_t handler_func, void *args) {
    ESP_LOGI(TAG, "Configuring Interrupt on Pin: %d", pin);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << pin),
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(pin, handler_func, args);
}

void switch_callback(void *arg) {
    if (gpio_get_level(ARM_SWITCH_PIN) == 0) {
        g_is_armed = false;
    } else {
        g_is_armed = true;
    }
}

void switch_init(void){
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ARM_SWITCH_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    const esp_timer_create_args_t timer_args = {
        .callback = &switch_callback,
        .name = "switch_sampler"
    };
    esp_timer_handle_t timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, 20000));
}

void dump_i2c_device(uint8_t addr) {
    printf("\n--- DUMP ZARIZENI 0x%02X ---\n", addr);
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    
    for (int reg = 0; reg < 0x20; reg += 16) {
        printf("%02X: ", reg);
        for (int i = 0; i < 16; i++) {
            uint8_t val = 0;
            esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, addr, (uint8_t[]){reg + i}, 1, &val, 1, pdMS_TO_TICKS(100));
            if (err == ESP_OK) {
                printf("%02X ", val);
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }
}