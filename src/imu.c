#include "imu.h"
#include "utils.h"
#include "u8g2_esp32_hal.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <math.h>
#include <freertos/semphr.h>

#define MPU9250_ADDR         0x68
#define MPU9250_PWR_MGMT_1   0x6B
#define MPU9250_SENSOR_START 0x3B 

// Sensor Scaling Constants
#define ACCEL_SCALE_2G       16384.0f
#define GYRO_SCALE_250DPS    131.0f
#define RAD_TO_DEG           57.29578f

static const char* TAG = "IMU";

// Internal State
static Kalman_t kalmanX, kalmanY;
static bool initialized = false;
static int64_t last_time = 0;

// Internal Data
static float accel_offset_x = 0.0f;
static float accel_offset_y = 0.0f;
static float gyro_offset_z = 0.0f;

extern SemaphoreHandle_t i2c_mutex;

// --- Start of program logic ---

void kalman_init(Kalman_t *k) {
    k->Q_angle = 0.001f;
    k->Q_bias = 3.5f;
    k->R_measure = 3.5f;
    k->angle = 0.0f;
    k->bias = 0.0f;
    
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            k->P[i][j] = 0.0f;
        }
    }
}

float kalman_get_angle(Kalman_t *k, float new_angle, float new_rate, float dt) {
    float rate = new_rate - k->bias;
    k->angle += dt * rate;

    k->P[0][0] += dt * (dt * k->P[1][1] - k->P[0][1] - k->P[1][0] + k->Q_angle);
    k->P[0][1] -= dt * k->P[1][1];
    k->P[1][0] -= dt * k->P[1][1];
    k->P[1][1] += k->Q_bias * dt;

    float S = k->P[0][0] + k->R_measure;
    float K[2] = { k->P[0][0] / S, k->P[1][0] / S };

    float y = new_angle - k->angle;
    k->angle += K[0] * y;
    k->bias += K[1] * y;

    float P00_temp = k->P[0][0];
    float P01_temp = k->P[0][1];

    k->P[0][0] -= K[0] * P00_temp;
    k->P[0][1] -= K[0] * P01_temp;
    k->P[1][0] -= K[1] * P00_temp;
    k->P[1][1] -= K[1] * P01_temp;

    return k->angle;
}

void imu_calibrate(void) {
    ESP_LOGI(TAG, "Starting calibration... DO NOT MOVE SENSOR!");
    float sum_x = 0, sum_y = 0, sum_gz = 0;
    const int samples = 200;

    accel_offset_x = 0.0f;
    accel_offset_y = 0.0f;
    gyro_offset_z = 0.0f;

    for (int i = 0; i < samples; i++) {
        // Temporarily read data with zeroed offset
        imu_data_t raw = imu_get_data(); 
        sum_x += raw.acc_x;
        sum_y += raw.acc_y;
        sum_gz += raw.gyro_z;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    accel_offset_x = sum_x / samples;
    accel_offset_y = sum_y / samples;
    gyro_offset_z = sum_gz / samples;
    
    ESP_LOGI(TAG, "Calibration complete! Offsets: X:%.2f, Y:%.2f, Z:%.2f", 
             accel_offset_x, accel_offset_y, gyro_offset_z);
}

void imu_init(void) {    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU9250_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, MPU9250_PWR_MGMT_1, true);
    i2c_master_write_byte(cmd, 0x00, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU9250! Error: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "MPU9250 Woken Up Successfully");
    }
}

imu_data_t imu_get_data(void) {
    if (!initialized) {
        kalman_init(&kalmanX);
        kalman_init(&kalmanY);
        last_time = esp_timer_get_time();
        initialized = true;
    }

    uint8_t data[14];
    imu_data_t imu = {0};

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (MPU9250_ADDR << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, MPU9250_SENSOR_START, true);

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (MPU9250_ADDR << 1) | I2C_MASTER_READ, true);
        
        i2c_master_read(cmd, data, 13, I2C_MASTER_ACK);
        i2c_master_read_byte(cmd, data + 13, I2C_MASTER_NACK);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);

        xSemaphoreGive(i2c_mutex);

        if (ret == ESP_OK) {
            int16_t raw_ax = (data[0] << 8) | data[1];
            int16_t raw_ay = (data[2] << 8) | data[3];
            int16_t raw_az = (data[4] << 8) | data[5];

            int16_t raw_gx = (data[8] << 8) | data[9];
            int16_t raw_gy = (data[10] << 8) | data[11];
            int16_t raw_gz = (data[12] << 8) | data[13];

            // Convert to physical units
            float ax = (float)raw_ax / ACCEL_SCALE_2G;
            float ay = (float)raw_ay / ACCEL_SCALE_2G;
            float az = (float)raw_az / ACCEL_SCALE_2G;

            float gx = (float)raw_gx / GYRO_SCALE_250DPS;
            float gy = (float)raw_gy / GYRO_SCALE_250DPS;
            float gz = (float)raw_gz / GYRO_SCALE_250DPS;

            // Calculate angle from accelerometer (converted to degrees)
            float roll  = atan2f(ay, az) * RAD_TO_DEG;
            float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;

            int64_t now = esp_timer_get_time();
            float dt = (now - last_time) / 1000000.0f;
            last_time = now;

            // Apply Kalman filter
            imu.acc_x = kalman_get_angle(&kalmanX, roll, gx, dt) - accel_offset_x;
            imu.gyro_x = gx - kalmanX.bias; 

            imu.acc_y = kalman_get_angle(&kalmanY, pitch, gy, dt) - accel_offset_y;
            imu.gyro_y = gy - kalmanY.bias; 
            
            imu.gyro_z = gz - gyro_offset_z;

            // Deadzone filtering
            if (fabsf(imu.acc_x) < 0.05f) imu.acc_x = 0.0f;
            if (fabsf(imu.acc_y) < 0.05f) imu.acc_y = 0.0f;
        }
    } else {
        ESP_LOGE(TAG, "I2C Mutex timeout - could not read IMU");
    }
    return imu;
}