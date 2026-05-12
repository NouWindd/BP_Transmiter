#ifndef MESSAGE_UTILS_H
#define MESSAGE_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_log.h"
#include "protocol.h"

/* --- Macros & Constants --- */
#define WIFIESPNOW_ALEN 6
#define GET_MSG_CHECKSUM(msg_ptr) \
    calculate_checksum((const uint8_t*)(msg_ptr), sizeof(*(msg_ptr)) - 1)

/* --- Message Container Structure --- */
typedef struct {
    uint8_t mac[WIFIESPNOW_ALEN];
    uint8_t len;
    union {
        uint8_t type;
        uint8_t payload[PAYLOAD_SIZE_MAX];
    } data; // In C, named unions are safer and more standard
} message_container_t;

/* --- Utility Functions --- */
static inline uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint8_t csum = 0x55;
    for (size_t i = 0; i < len; i++) {
        csum ^= data[i];
    }
    return csum;
}

/* --- Debug Function --- */
static inline void debug_message(const uint8_t *mac, const uint8_t *buf, size_t count) {
    if (mac != NULL) {
        ESP_LOGI("Protocol", "MAC: %02X:%02X:%02X:%02X:%02X:%02X | Len: %d", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], (int)count);
    }
}

#endif // MESSAGE_UTILS_H