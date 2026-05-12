#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "EspNowRcLink/protocol.h"

/* --- Macros & Constants --- */
#define BOX_LEFT_X  10
#define BOX_RIGHT_X 88
#define BOX_Y       20
#define BOX_SIZE    30

/* --- Data Types --- */
typedef enum {
    SCR_MAIN_MENU,
    SCR_FLIGHT_HUD,
    SCR_CALIBRATE,
    SCR_LINK_STATS
} screen_t;

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
} position_data_t;

/* --- Global Variables --- */
extern screen_t g_current_screen;
extern bool g_is_armed;
extern int g_menu_cursor;
extern float g_battery_voltage;
extern uint8_t g_signal_strength;
extern position_data_t g_stick_pos;
extern int8_t g_rssi;

/* --- Function Prototypes --- */
/**
 * @brief Initialize the OLED display driver.
 */
void display_init(void);

/**
 * @brief Performs a single refresh of the UI based on current system state.
 */
void display_update(void);

#endif // DISPLAY_H