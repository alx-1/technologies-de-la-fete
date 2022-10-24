#ifndef ESP_OSC_H
#define ESP_OSC_H

#include <stdbool.h>
#include <stdint.h>
#include <lwip/netdb.h>

#include <tinyosc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t *buf;
  uint16_t buf_len;
  int socket;
  struct sockaddr_in dest;
} esp_osc_client_t;

bool esp_osc_client_init(esp_osc_client_t *client, uint16_t size);

void esp_osc_client_select(esp_osc_client_t *client, const char *address, uint16_t port);

bool esp_osc_client_send(esp_osc_client_t *client, const char *topic, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif  // ESP_OSC_H
