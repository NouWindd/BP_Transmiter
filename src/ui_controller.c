#include "ui_controller.h"
#include "joystick.h"
#include "display.h"
#include "imu.h"
#include <esp_log.h>

#define ALPHA 0.2f
#define SENSITIVITY 2000.0f // Adjust to map angle to pixel range

//static const char* TAG = "UI_CTRL";

virtual_joy_t g_gyro_joy = {2048.0f, 2048.0f};
virtual_joy_t g_joy = {2048.0f, 2048.0f};

float filtered_x = 2048.0f;
float filtered_y = 2048.0f;


// ---Start of program logic---

/**
 * @brief Apply a low-pass filter to smooth out the input values.
 * @param filtered_value Pointer to the current filtered value (will be updated)
 * @param new_value The new raw input value to incorporate
 * @param alpha Smoothing factor (0 < alpha < 1), where higher values give more weight to the new value
 */
void low_pass_filter(float* filtered_value, float new_value, float alpha) {
    *filtered_value = (alpha * new_value) + ((1.0f - alpha) * (*filtered_value));
}

void parse_joystick_and_update_joy(joystick_data_t joy) {
    int offset_x = (joy.x * (BOX_SIZE - 3)) / 4095;
    int offset_y = (joy.y * (BOX_SIZE - 3)) / 4095;

    // Update global stick position for display
    g_stick_pos.x2 = BOX_RIGHT_X + 1 + offset_x; 
    g_stick_pos.y2 = BOX_Y + 1 + offset_y;
}

void parse_imu_and_update_joy(imu_data_t imu) {

    float max_angle = 60.0f;

    float virt_roll = 2048.0f + (imu.acc_x * (2048.0f / max_angle));
    // Pitch (imu.acc_y) mapujeme na Y osu pravého joysticku
    float virt_pitch = 2048.0f - (imu.acc_y * (2048.0f / max_angle));

    if (virt_roll < 0) virt_roll = 0;
    if (virt_roll > 4095) virt_roll = 4095;
    if (virt_pitch < 0) virt_pitch = 0;
    if (virt_pitch > 4095) virt_pitch = 4095;

    g_gyro_joy.virt_x = virt_roll;
    g_gyro_joy.virt_y = virt_pitch;

    // Map virtual joystick values to pixel offsets for display
    int offset_x = (virt_roll * (BOX_SIZE - 3)) / 4095;
    int offset_y = (virt_pitch * (BOX_SIZE - 3)) / 4095;

    g_stick_pos.x1 = BOX_LEFT_X + 1 + offset_x;
    g_stick_pos.y1 = BOX_Y + 1 + offset_y;
}

void ui_controller_update(void) {
    imu_data_t imu = imu_get_data();
    joystick_data_t joy = joystick_get_data();
    g_joy.virt_x = joy.x;
    g_joy.virt_y = joy.y;

    parse_joystick_and_update_joy(joy);
    parse_imu_and_update_joy(imu);

    if (g_current_screen == SCR_MAIN_MENU) {
        static bool moved = false;
        if (joy.y > 3000 && !moved) {
            g_menu_cursor = (g_menu_cursor + 1) % 3;
            moved = true;
        } else if (joy.y < 1000 && !moved) {
            g_menu_cursor = (g_menu_cursor - 1 + 3) % 3;
            moved = true;
        } else if (joy.y > 1000 && joy.y < 3000) {
            moved = false;
        }
    }

    static bool btn_was_pressed = false;
    if (joy.button_level == 0 && !btn_was_pressed) {
        if (g_current_screen == SCR_MAIN_MENU && g_menu_cursor == 0) {
            g_current_screen = SCR_FLIGHT_HUD;
        } else if (g_current_screen == SCR_MAIN_MENU && g_menu_cursor == 1) {
            g_current_screen = SCR_CALIBRATE;
            display_update();

            imu_calibrate();

             g_current_screen = SCR_MAIN_MENU;
        } else if (g_current_screen == SCR_MAIN_MENU && g_menu_cursor == 2) {
            g_current_screen = SCR_LINK_STATS;
        } else {
            g_current_screen = SCR_MAIN_MENU;
        }
    }
    btn_was_pressed = (joy.button_level == 0);
}