#ifndef IMU_H
#define IMU_H

#include <stdint.h>

/* --- Data Types --- */
typedef struct {
    float acc_x, acc_y, acc_z;
    float gyro_x, gyro_y, gyro_z;
    float temp;
} imu_data_t;

typedef struct {
    float virt_x;
    float virt_y;
} virtual_joy_t;

typedef struct {
    float Q_angle;
    float Q_bias;
    float R_measure;
    float angle;
    float bias;
    float P[2][2];
} Kalman_t;

/* --- Global Variables --- */
extern virtual_joy_t g_gyro_joy;
extern virtual_joy_t g_joy;

/* --- Function Prototypes --- */
/**
 * @brief Initialize the MPU9250. Assumes I2C bus is already initialized.
 */
void imu_init(void);

/**
 * @brief Fetch the latest scaled readings from the sensor.
 */
imu_data_t imu_get_data(void);

/**
 * @brief Calibrates the IMU offsets. DO NOT move the sensor during this.
 */
void imu_calibrate(void);

#endif // IMU_H