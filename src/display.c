#include "display.h"
#include <esp_log.h>
#include <u8g2.h>
#include "u8g2_esp32_hal.h"
#include <driver/i2c.h>
#include <stdint.h>
#include "utils.h"
#include "freertos/semphr.h"
#include "joystick.h"

static u8g2_t u8g2;
screen_t g_current_screen = SCR_MAIN_MENU;
bool g_is_armed = false;
int g_menu_cursor = 0;
uint8_t g_signal_strength = 0;
position_data_t g_stick_pos = {0, 0, 0, 0};
extern SemaphoreHandle_t i2c_mutex;

static void draw_signal_icon(int x, int y, int rssi_pct) {
    for (int i = 0; i < 4; i++) {
        if (rssi_pct > (i * 25)) {
            u8g2_DrawVLine(&u8g2, x + (i * 3), y - (i * 2), i * 2 + 1);
        }
    }
}

static void draw_header(void) {
    u8g2_SetFont(&u8g2, u8g2_font_u8glib_4_tr);
    u8g2_DrawFrame(&u8g2, 0, 1, 10, 5);
    u8g2_DrawPixel(&u8g2, 10, 3);

    char volt_str[10];
    snprintf(volt_str, sizeof(volt_str), "%.1fV", (float)(g_bat_raw * 2) / 4095 * 3.3);

    u8g2_DrawStr(&u8g2, 12, 6, volt_str);

    draw_signal_icon(38, 6, g_signal_strength);

    if (g_is_armed) {
        u8g2_DrawBox(&u8g2, 60, 0, 30, 9);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawStr(&u8g2, 62, 7, "ARMED");
        u8g2_SetDrawColor(&u8g2, 1);
    } else {
        u8g2_DrawFrame(&u8g2, 60, 0, 30, 9);
        u8g2_DrawStr(&u8g2, 63, 7, "DISR");
    }

    u8g2_DrawStr(&u8g2, 100, 6, "D:");
    u8g2_DrawStr(&u8g2, 108, 6, "12.6V");

    u8g2_DrawHLine(&u8g2, 0, 10, 128);
}

void display_init(void) {
    u8g2_esp32_hal_t hal = U8G2_ESP32_HAL_DEFAULT;
    
    hal.bus.i2c.sda = U8G2_ESP32_HAL_UNDEFINED; 
    hal.bus.i2c.scl = U8G2_ESP32_HAL_UNDEFINED;
    
    u8g2_esp32_hal_init(hal);

    u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
        u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

void display_update(void) {
    static int8_t rssi_history[100] = {0};
    
    u8g2_ClearBuffer(&u8g2);
    draw_header();

    if (g_current_screen == SCR_MAIN_MENU) {
        const char* apps[] = {" > START FLIGHT", " > CALIBRATE IMU", " > TELEMETRY"};
        u8g2_SetFont(&u8g2, u8g2_font_7x14_tr);
        for(int i = 0; i < 3; i++) {
            int y = 30 + (i * 15);
            if (g_menu_cursor == i) u8g2_DrawTriangle(&u8g2, 2, y-10, 2, y, 8, y-5);
            u8g2_DrawStr(&u8g2, 2, y, apps[i]);
        }
    } else if (g_current_screen == SCR_FLIGHT_HUD) {
        u8g2_DrawFrame(&u8g2, 10, 20, 30, 30);
        u8g2_DrawFrame(&u8g2, 88, 20, 30, 30);
        u8g2_DrawDisc(&u8g2, g_stick_pos.x2, g_stick_pos.y2, 2, U8G2_DRAW_ALL);
        u8g2_DrawDisc(&u8g2, g_stick_pos.x1, g_stick_pos.y1, 2, U8G2_DRAW_ALL);
    } else if (g_current_screen == SCR_CALIBRATE) {
        u8g2_SetFont(&u8g2, u8g2_font_7x14_tr);
        u8g2_DrawStr(&u8g2, 15, 30, "CALIBRATING...");
        
        u8g2_SetFont(&u8g2, u8g2_font_u8glib_4_tr);
        u8g2_DrawStr(&u8g2, 10, 45, "DO NOT MOVE DEVICE");

        u8g2_DrawFrame(&u8g2, 14, 50, 100, 6);
    } else if (g_current_screen == SCR_LINK_STATS) {
        u8g2_SetFont(&u8g2, u8g2_font_4x6_tr); 
        char buf[32];

        // Box dimensions and position for perfect centering
        int box_x = 14;
        int box_y = 22;
        int box_w = 100;
        int box_h = 35;

        snprintf(buf, sizeof(buf), "RSSI: %d dBm", g_rssi);
        u8g2_DrawStr(&u8g2, box_x, box_y - 4, buf);

        for (int i = 0; i < 99; i++) {
            rssi_history[i] = rssi_history[i + 1];
        }
        rssi_history[99] = g_rssi; // Add the newest value to the end

        u8g2_DrawFrame(&u8g2, box_x, box_y, box_w, box_h);
        
        // Draw the RSSI history graph
        for (int i = 0; i < 99; i++) {
            int y1 = (box_y + box_h) - ((rssi_history[i] + 100) * box_h / 100);
            int y2 = (box_y + box_h) - ((rssi_history[i+1] + 100) * box_h / 100);
            
            if (y1 < box_y) y1 = box_y; 
            if (y1 > box_y + box_h) y1 = box_y + box_h;
            if (y2 < box_y) y2 = box_y; 
            if (y2 > box_y + box_h) y2 = box_y + box_h;

            u8g2_DrawLine(&u8g2, box_x + i, y1, box_x + i + 1, y2);
        }
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        u8g2_SendBuffer(&u8g2);
        xSemaphoreGive(i2c_mutex);
    }
}