#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/* --- Macros & Constants --- */
#define RC_DATA   0x01 // RC Channels (0x00-0x0f tx->rx)
#define FC_ALIVE  0x10 // FC Alive (0x10-0x1f rx->tx)
#define FC_DATA   0x11 // FC Telemetry
#define PAIR_REQ  0xfe // Pair Request (beacon rx->tx)
#define PAIR_RES  0xff // Pair Response (pair tx->rx)

#define PAYLOAD_SIZE_MIN 2
#define PAYLOAD_SIZE_MAX sizeof(message_rc_t)

static const size_t WIFI_CHANNEL_MIN = 1;
static const size_t WIFI_CHANNEL_MAX = 13;
static const size_t WIFI_CHANNEL_DEFAULT = 7;

static const size_t RC_CHANNEL_MIN = 0;
static const size_t RC_CHANNEL_MAX = 7;

static const size_t PWM_INPUT_MIN = 880;
static const size_t PWM_INPUT_CENTER = 1500;
static const size_t PWM_INPUT_MAX = 2120;

static const size_t LINK_DISCOVERY_INTERVAL_MS = 200;
static const size_t LINK_BEACON_INTERVAL_MS = 50;
static const size_t LINK_ALIVE_INTERVAL_MS = 1000;

/* --- Data Types --- */
typedef uint8_t message_type_t;

typedef struct __attribute__((packed)) {
    uint8_t type; // Set to RC_DATA
    int16_t ch1;
    int16_t ch2;
    int16_t ch3;
    int16_t ch4;
    int8_t ch5;
    int8_t ch6;
    int8_t ch7;
    int8_t ch8;
    uint8_t csum;
} message_rc_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t csum;
} message_alive_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    int16_t ch1;
    int16_t ch2;
    int8_t ch3;
    int8_t ch4;
    uint8_t csum;
} message_fc_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t channel;
    uint8_t csum;
} message_pair_request_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t csum;
} message_pair_response_t;

#endif // PROTOCOL_H