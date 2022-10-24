#include <stdlib.h>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <esp_osc.h>

#define WIFI_SSID "SMNX_IoT"
#define WIFI_PASS "y9Q@&h5bLIo!}bAlhI6P@HlL"

#define OSC_ADDRESS "192.168.30.130"
#define OSC_PORT 8000

esp_osc_client_t client;

static void process() {
  for (;;) {
    // send message
    esp_osc_client_send(&client, "foo", "if", 42, 3.14);

    // delay
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

static void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case SYSTEM_EVENT_STA_START:
        // connect to ap
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());

        break;

      case SYSTEM_EVENT_STA_GOT_IP:
        break;

      case SYSTEM_EVENT_STA_DISCONNECTED:
        // reconnect Wi-Fi
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());

        break;

      default:
        break;
    }
  }
}

void app_main() {
  // initialize NVS flash
  ESP_ERROR_CHECK(nvs_flash_init());

  // initialize networking
  ESP_ERROR_CHECK(esp_netif_init());

  // create default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // enable Wi-Fi
  esp_netif_create_default_wifi_sta();

  // initialize Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // set Wi-Fi storage to ram
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  // set wifi mode
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // register event handlers
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

  // prepare Wi-Fi config
  wifi_config_t wifi_config = {.sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}};
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // start Wi-Fi
  ESP_ERROR_CHECK(esp_wifi_start());

  // prepare client
  esp_osc_client_init(&client, 1024);
  esp_osc_client_select(&client, OSC_ADDRESS, OSC_PORT);

  // create tasks
  xTaskCreatePinnedToCore(process, "process", 2048, NULL, 10, NULL, 1);
}
