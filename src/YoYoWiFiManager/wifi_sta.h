#ifndef wifi_sta_h
#define wifi_sta_h

#define ESP_WIFI_MAX_CONN_NUM  (10)       //TODO: is this right? This is the max for ESP32

typedef struct {
    uint8_t mac[6];
    ip4_addr_t ip;
} tcpip_adapter_sta_info_t;

typedef struct {
    tcpip_adapter_sta_info_t sta[ESP_WIFI_MAX_CONN_NUM];
    int num;
} tcpip_adapter_sta_list_t;

#endif