#include "joystick.h"
#include "hw_config.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_timer.h>

#define ADC_MAX_VAL  4095.0f
#define EXPO         0.5f

static const char* TAG = "JOYSTICK";

// Internal State
static adc_oneshot_unit_handle_t adc_handle;

// Internal Data
static int x_raw = 0;
static int y_raw = 0;
static int sw_state = 1;
static int last_sw_state = 1;

int g_bat_raw = 0;
bool g_button_clicked = false;

// --- Start of program logic ---

int apply_throttle_curve(int raw_val) {
    // Normalize to [0.0, 1.0]
    float x = (float)raw_val / ADC_MAX_VAL;

    float centered = x - 0.5f;
    float curved = (1.0f - EXPO) * centered + EXPO * (4.0f * centered * centered * centered);
    
    float final_val = curved + 0.5f;

    if (final_val > 1.0f) final_val = 1.0f;
    if (final_val < 0.0f) final_val = 0.0f;

    return (int)(final_val * ADC_MAX_VAL);
}

static void joystick_timer_callback(void *arg) {
    adc_oneshot_read(adc_handle, JOYSTICK_X_CHAN, (int*)&x_raw);
    adc_oneshot_read(adc_handle, JOYSTICK_Y_CHAN, (int*)&y_raw);
    adc_oneshot_read(adc_handle, BATTERY_ADC_CHAN, &g_bat_raw);
    
    int current_sw_state = gpio_get_level(JOYSTICK_SW_PIN);

    if (current_sw_state == 0 && last_sw_state == 1) {
        g_button_clicked = true;
    }
    
    sw_state = current_sw_state;
    last_sw_state = current_sw_state;
}

void joystick_init(void) {
    ESP_LOGI(TAG, "Initializing Joystick...");

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, JOYSTICK_X_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, JOYSTICK_Y_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHAN, &config));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << JOYSTICK_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Start Polling Timer (50Hz)
    const esp_timer_create_args_t timer_args = {
        .callback = &joystick_timer_callback,
        .name = "joystick_sampler"
    };
    esp_timer_handle_t timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, 20000));

    ESP_LOGI(TAG, "Joystick initialized (Polling at 50Hz)");
}

joystick_data_t joystick_get_data(void) {
    joystick_data_t data = {
        .x = 4095 - y_raw,
        .y = apply_throttle_curve(x_raw),
        .button_level = sw_state,
        .clicked = g_button_clicked,
    };

    g_button_clicked = false;
    return data;
}