#include <string.h>
#include "EspNowRcLink/transmitter.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "RC_LINK";
const uint8_t BCAST_PEER[WIFIESPNOW_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Internal Queue Constants
#define QUEUE_SIZE 10
static message_container_t msg_queue[QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
int8_t rssi = -100;

static uint32_t send_start_time = 0;

// Internal Helpers
static uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static int8_t encode_aux(int x) {
    if (x < (int)PWM_INPUT_MIN) x = PWM_INPUT_MIN;
    if (x > (int)PWM_INPUT_MAX) x = PWM_INPUT_MAX;
    x -= PWM_INPUT_CENTER;
    int rounding = x > 0 ? 2 : -2;
    return (int8_t)((x + rounding) / 5);
}

static void wifi_now_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        // Stop the timer
        uint32_t current_time = (uint32_t)esp_timer_get_time(); 
        uint32_t rtt_us = current_time - send_start_time;
        uint32_t one_way_latency_us = rtt_us / 2;
        
        ESP_LOGI(TAG, "Air Latency: %lu us", one_way_latency_us);
    } else {
        ESP_LOGW(TAG, "Packet delivery failed");
    }
}

static void wifi_now_rx_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len < PAYLOAD_SIZE_MIN || len > PAYLOAD_SIZE_MAX) return;
    
    // Checksum validation
    if (calculate_checksum(data, len - 1) != data[len - 1]) return;

    // Check if queue is full
    int next = (queue_head + 1) % QUEUE_SIZE;
    if (next == queue_tail) return; // Drop packet if full

    // Store in queue
    memcpy(msg_queue[queue_head].mac, recv_info->src_addr, WIFIESPNOW_ALEN);
    msg_queue[queue_head].len = len;
    memcpy(msg_queue[queue_head].data.payload, data, len);
    queue_head = next;
    rssi = recv_info->rx_ctrl->rssi;
}

// --- Public API Implementation ---

esp_err_t transmitter_init(transmitter_t *tx, bool enSoftAp) {
    memset(tx, 0, sizeof(transmitter_t));
    tx->wifi_channel = WIFI_CHANNEL_DEFAULT;
    tx->state = TX_STATE_DISCOVERING;
    tx->use_softap = enSoftAp;

    // WiFi Init (Required for ESP-NOW)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    if (enSoftAp) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_channel(tx->wifi_channel, WIFI_SECOND_CHAN_NONE);

    // ESP-NOW Init
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(wifi_now_rx_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(wifi_now_send_cb));

    // Add Broadcast Peer initially
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, BCAST_PEER, WIFIESPNOW_ALEN);
    peer_info.channel = tx->wifi_channel;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));

    return ESP_OK;
}

static void handle_discovery(transmitter_t *tx) {
    uint32_t now = millis();
    if (now >= tx->next_discovery_ms) {
        tx->wifi_channel++;
        if (tx->wifi_channel > WIFI_CHANNEL_MAX) tx->wifi_channel = WIFI_CHANNEL_MIN;
        
        esp_wifi_set_channel(tx->wifi_channel, WIFI_SECOND_CHAN_NONE);
        tx->next_discovery_ms = now + LINK_DISCOVERY_INTERVAL_MS;
        ESP_LOGD(TAG, "Scanning channel %d", tx->wifi_channel);
    }
}

static void handle_transmit(transmitter_t *tx) {
    if (tx->is_ready) {
        tx->channels.type = RC_DATA;
        tx->channels.csum = GET_MSG_CHECKSUM(&tx->channels);
        
        send_start_time = (uint32_t)esp_timer_get_time();

        esp_now_send(tx->peer_mac, (uint8_t*)&tx->channels, sizeof(message_rc_t));
        tx->is_ready = false;
    }
}

static void handle_received(transmitter_t *tx) {
    while (queue_tail != queue_head) {
        message_container_t *m = &msg_queue[queue_tail];

        tx->last_rx_ms = millis();

        switch (m->data.type) {
            case PAIR_REQ: {
                message_pair_request_t *pr = (message_pair_request_t*)m->data.payload;
                if (pr->channel >= WIFI_CHANNEL_MIN && pr->channel <= WIFI_CHANNEL_MAX) {
                    tx->wifi_channel = pr->channel;
                    esp_wifi_set_channel(tx->wifi_channel, WIFI_SECOND_CHAN_NONE);
                    tx->state = TX_STATE_TRANSMITTING;
                    
                    memcpy(tx->peer_mac, m->mac, WIFIESPNOW_ALEN);
                    
                    // Add the specific peer to ESP-NOW list
                    if (!esp_now_is_peer_exist(tx->peer_mac)) {
                        esp_now_peer_info_t peer_info = {0};
                        memcpy(peer_info.peer_addr, tx->peer_mac, WIFIESPNOW_ALEN);
                        peer_info.channel = tx->wifi_channel;
                        esp_now_add_peer(&peer_info);
                    }

                    // Send response
                    message_pair_response_t res = { .type = PAIR_RES };
                    res.csum = GET_MSG_CHECKSUM(&res);
                    esp_now_send(tx->peer_mac, (uint8_t*)&res, sizeof(res));
                    ESP_LOGI(TAG, "Paired with %02X:%02X...", tx->peer_mac[0], tx->peer_mac[1]);
                }
                break;
            }
            case FC_DATA:
                memcpy(&tx->sensors, m->data.payload, sizeof(message_fc_t));
                break;
            default:
                break;
        }
        queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    }
}

void transmitter_update(transmitter_t *tx) {
    switch (tx->state) {
        case TX_STATE_DISCOVERING:
            handle_discovery(tx);
            rssi = -100;
            break;
        case TX_STATE_TRANSMITTING:
            handle_transmit(tx);

            uint32_t now = millis();

            if (now - tx->last_rx_ms > 10000) {
                ESP_LOGW(TAG, "Link timeout, returning to discovery");
                tx->state = TX_STATE_DISCOVERING;
            }

            break;
        default:
            break;
    }
    handle_received(tx);
}

void transmitter_set_channel(transmitter_t *tx, size_t c, uint16_t value) {
    if (value < PWM_INPUT_MIN) value = PWM_INPUT_MIN;
    if (value > PWM_INPUT_MAX) value = PWM_INPUT_MAX;
    if (c > RC_CHANNEL_MAX) return;

    switch(c) {
        case 0: tx->channels.ch1 = value; break;
        case 1: tx->channels.ch2 = value; break;
        case 2: tx->channels.ch3 = value; break;
        case 3: tx->channels.ch4 = value; break;
        case 4: tx->channels.ch5 = encode_aux(value); break;
        case 5: tx->channels.ch6 = encode_aux(value); break;
        case 6: tx->channels.ch7 = encode_aux(value); break;
        case 7: tx->channels.ch8 = encode_aux(value); break;
    }
}

void transmitter_commit(transmitter_t *tx) {
    tx->is_ready = true;
}

int transmitter_get_sensor(const transmitter_t *tx, size_t id) {
    switch(id) {
        case 0: return tx->sensors.ch1;
        case 1: return tx->sensors.ch2;
        case 2: return tx->sensors.ch3;
        case 3: return tx->sensors.ch4;
        default: return -1;
    }
}