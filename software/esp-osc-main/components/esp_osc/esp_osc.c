#include <stdio.h>
#include <esp_log.h>

#include "esp_osc.h"

#define TAG "esp-osc"

bool esp_osc_client_init(esp_osc_client_t *client, uint16_t size) {
  // free existing memory
  if (client->buf != NULL) {
    free(client->buf);
    client->buf = NULL;
    client->buf_len = 0;
  }

  // allocate memory
  client->buf = malloc(size);
  client->buf_len = size;

  // close existing socket
  if (client->socket != 0) {
    close(client->socket);
    client->socket = 0;
  }

  // create socket
  client->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (client->socket <= 0) {
    ESP_LOGE(TAG, "failed to create socket (%d)", errno);
    client->socket = 0;
    return false;
  }

  return true;
}

void esp_osc_client_select(esp_osc_client_t *client, const char *address, uint16_t port) {
  // prepare address
  client->dest.sin_addr.s_addr = inet_addr(address);
  client->dest.sin_family = AF_INET;
  client->dest.sin_port = htons(port);
}

bool esp_osc_client_send(esp_osc_client_t *client, const char *topic, const char *format, ...) {
  // prepare message
  va_list args;
  va_start(args, format);
  uint32_t length = tosc_vwrite((char *)client->buf, client->buf_len, topic, format, args);
  va_end(args);

  // send message
  int ret = sendto(client->socket, client->buf, length, 0, (struct sockaddr *)&client->dest, sizeof(client->dest));
  if (ret < 0) {
    ESP_LOGE(TAG, "failed to send message (%d)", errno);
    return false;
  }

  return true;
}
