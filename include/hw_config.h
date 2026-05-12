#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <driver/gpio.h>
#include <hal/adc_types.h>

/* ========================================================================= */
/*                              I2C CONFIGURATION                            */
/* ========================================================================= */
#define I2C_SDA_PIN             1
#define I2C_SCL_PIN             2
#define I2C_BUS_FREQ_HZ         400000

/* ========================================================================= */
/*                           JOYSTICK CONFIGURATION                          */
/* ========================================================================= */
#define JOYSTICK_SW_PIN         GPIO_NUM_6
#define JOYSTICK_X_CHAN         ADC_CHANNEL_3 
#define JOYSTICK_Y_CHAN         ADC_CHANNEL_4

/* ========================================================================= */
/*                            SWITCH & SENSORS                               */
/* ========================================================================= */
#define ARM_SWITCH_PIN          GPIO_NUM_21
#define BATTERY_ADC_CHAN        ADC_CHANNEL_7

#endif // HW_CONFIG_H