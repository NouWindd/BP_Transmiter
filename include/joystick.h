#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

/* --- Data Types --- */
typedef struct {
    int x;
    int y;
    int button_level;
    bool clicked;
} joystick_data_t;

/* --- Global Variables --- */
extern int g_bat_raw;
extern bool g_button_clicked;

/* --- Function Prototypes --- */
/**
 * @brief Initialize ADC and GPIO for the joystick using board configuration.
 */
void joystick_init(void);

/**
 * @brief Get the latest formatted joystick readings.
 */
joystick_data_t joystick_get_data(void);

#endif // JOYSTICK_H