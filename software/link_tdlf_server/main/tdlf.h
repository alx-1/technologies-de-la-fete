#include <ableton/Link.hpp>
#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_event.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <protocol_examples_common.h>
#include "driver/uart.h"
#include "esp_timer.h" // for tap tempo
#include "esp_sleep.h"
#include <chrono> // for setTempo()

extern "C" {
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h> // from the station_example_main example
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <stdbool.h> 
#include "lwip/inet.h" // inet.pton() 

#include <stdlib.h> // from the smart_config example
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "freertos/queue.h"
#include "esp_log.h"

///// MDNS //////
#include "mdns.h"//
#include "netdb.h" //

///// OSC ///////
#include <tinyosc.h> // still to test for speed !!

}