#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_now.h"
#include "protocol.h"
#include "common.h"

/* --- Data Types --- */
typedef enum {
    TX_STATE_DISCOVERING,
    TX_STATE_TRANSMITTING,
} transmitter_state_t;

typedef struct {
    message_rc_t channels;
    message_fc_t sensors;
    size_t wifi_channel;
    uint8_t peer_mac[WIFIESPNOW_ALEN];
    uint32_t next_discovery_ms;
    transmitter_state_t state;
    bool is_ready;
    bool use_softap;
    uint32_t last_rx_ms;
} transmitter_t;

/* --- Global Variables --- */
extern const uint8_t BCAST_PEER[WIFIESPNOW_ALEN];
extern int8_t rssi;

/* --- Function Prototypes --- */
/**
 * @brief Initialize the transmitter module and ESP-NOW.
 * @param tx Pointer to a transmitter_t struct to initialize.
 * @param enSoftAp If true, the ESP will create a SoftAP for pairing instead of using the default WiFi channel.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t transmitter_init(transmitter_t *tx, bool enSoftAp);

/**
 * @brief Update the transmitter module.
 * @param tx Pointer to a transmitter_t struct to update.
 */
void transmitter_update(transmitter_t *tx);

/**
 * @brief Set a specific RC channel value (0-7).
 * @param tx Pointer to a transmitter_t struct.
 * @param channel_idx Index of the channel to set (0-7).
 * @param value PWM value to set (typically 880-2120).
 */
void transmitter_set_channel(transmitter_t *tx, size_t channel_idx, uint16_t value);

/**
 * @brief Get sensor data received from the FC.
 * @param tx Pointer to a transmitter_t struct.
 * @param sensor_id Index of the sensor to get.
 * @return The sensor value or an error code.
 */
int transmitter_get_sensor(const transmitter_t *tx, size_t sensor_id);

/**
 * @brief Commit changes / Trigger a manual send if needed.
 * @param tx Pointer to a transmitter_t struct.
 */
void transmitter_commit(transmitter_t *tx);

/**
 * @brief De-initialize and stop ESP-NOW.
 * @param tx Pointer to a transmitter_t struct.
 */
void transmitter_deinit(transmitter_t *tx);

#endif // TRANSMITTER_H