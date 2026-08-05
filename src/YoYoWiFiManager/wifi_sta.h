#ifndef wifi_sta_h
#define wifi_sta_h

#define ESP_WIFI_MAX_CONN_NUM  (10)       //TODO: is this right? This is the max for ESP32

//This library's own station-list shape, used on both ESP8266 and ESP32 - deliberately
//not tied to any particular SDK struct, since ESP-IDF has already removed/renamed its
//own equivalent (tcpip_adapter_sta_list_t) once - see #61:
typedef struct {
    uint8_t mac[6];
    ip4_addr_t ip;
} YoYoStaInfo;

typedef struct {
    YoYoStaInfo sta[ESP_WIFI_MAX_CONN_NUM];
    int num;
} YoYoStaList;

#endif
