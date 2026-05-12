#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

/* --- Function Prototypes --- */
/**
 * @brief Initialize the I2C bus.
 * @param sda_pin GPIO number for I2C SDA line
 * @param scl_pin GPIO number for I2C SCL line
 */
void utils_init_i2c_bus(void);

/**
 * @brief Print all devices found on the I2C bus.
 */
void get_i2c_devices(void);

/**
 * @brief Dumps the first 32 registers of a specific I2C device.
 */
void dump_i2c_device(uint8_t addr);

/**
 * @brief Initialize the switch GPIO and its periodic sampling timer.
 */
void switch_init(void);

/**
 * @brief Initialize the GPIO interrupt service. Call this once at startup.
 */
void init_gpio_interrupt_service(void);

/**
 * @brief Add an interrupt handler for a specific GPIO pin.
 */
void add_interrupt(gpio_num_t pin, gpio_isr_t handler_func, void *args);

#endif // UTILS_H