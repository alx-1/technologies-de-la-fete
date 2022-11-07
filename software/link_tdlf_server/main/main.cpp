////// tdlf server ///// receive note, midi channel info through UDP + send midi through serial // Save, Load sequences
////// ESP32 Ableton Link  // midi clock // BPM (+ - )// Start/Stop 
////// Smart config NVS enabled to set the wifi credentials from ESPTouch app if no IP is attributed

bool startStopState = false; // l'état local
bool seqToLoad = false; // to know when to send the loaded sequence to clients

#include "tdlf.h" 
#include "sockets.h"
#include "i2c_display.h"
#include "ledc.h"
#include "touch_interactions.h"
#include "mdns_wifi_cfg.h"
#include "midi_link.h"

// bool mstr[79] = {}; // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
// ** Could be a confusion with "char mstr[16]; // char mstr[64]" defined in sockets.h !! **

extern "C" { void app_main()
{ 
  //Initialize and read in wifi credentials from NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
    }
  ESP_ERROR_CHECK(ret);
    
  /// Error check NVS credentials
  esp_err_t err;
	size_t len;
	// Open
	nvs_handle wificfg_nvs_handler;
	err = nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
        
  if (err != ESP_OK) {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    skipNVSRead = true;
    } else {
	    nvs_get_str(wificfg_nvs_handler, "wifi_ssid", NULL, &len);
      char* lssid = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
      err = nvs_get_str(wificfg_nvs_handler, "wifi_ssid", lssid, &len);
	    switch (err) {
	      case ESP_OK:
	        break;
	        case ESP_ERR_NVS_NOT_FOUND:
	          printf("Key wifi_ssid is not initialized yet!\n");
            skipNVSRead = true;
	          break;
	        default :
	          printf("Error (%s) reading wifi_ssid size!\n", esp_err_to_name(err));
	          skipNVSRead = true;
            break;
	        }
      }
  nvs_close(wificfg_nvs_handler);

  if(skipNVSRead){
    //////// WRITING DUMMY VALUES TO NVS FROM CLEAN SHEET //////
    // nvs_handle wificfg_nvs_handler;
    nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
    nvs_set_str(wificfg_nvs_handler,"wifi_ssid","testing");
    nvs_set_str(wificfg_nvs_handler,"wifi_password","onetwo");
    nvs_commit(wificfg_nvs_handler); 
    nvs_close(wificfg_nvs_handler); 
    ////// END NVS ///// 
  }

  /// NVS READ CREDENTIALS ///
  nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
  nvs_get_str(wificfg_nvs_handler, "wifi_ssid", NULL, &len);
  char* lssid = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
  nvs_get_str(wificfg_nvs_handler, "wifi_ssid", lssid, &len);
  nvs_get_str(wificfg_nvs_handler, "wifi_password", NULL, &len);
  char* lpassword = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
  nvs_get_str(wificfg_nvs_handler, "wifi_password", lpassword, &len); // esp_err_tnvs_get_str(nvs_handle_thandle, const char *key, char *out_value, size_t *length)
  nvs_close(wificfg_nvs_handler);
  ESP_LOGI(NVS_TAG,"INIT :%s",lssid); 
  ESP_LOGI(NVS_TAG,"INIT :%s",lpassword); 
  ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
  wifi_init_sta();

  if (goSMART == true){
    //ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 5000
    xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, &xHandle);
  }

  ESP_LOGI(WIFI_TAG, "weeeeeiiiiiiird ##################");
  tcpip_adapter_init();
  
  //ESP_ERROR_CHECK(esp_event_loop_create_default());
  
  initialise_mdns(); // initialise mDNS

  // #if defined USE_SOCKETS // yep sockette server //
  xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
  
  ESP_LOGI(TOUCH_TAG, "Initializing touch pad");     // Initialize touch pad peripheral, it will start a timer to run a filter
  touch_pad_init();
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);   // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);// Set reference voltage for charging/discharging
  tp_example_touch_pad_init();    // Init touch pad IO
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD); // Initialize and start a software filter to detect slight change of capacitance.
  tp_example_set_thresholds();     // Set thresh hold
  touch_pad_isr_register(tp_example_rtc_intr, NULL); // Register touch interrupt ISR
  xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL); // Start a task to show what pads have been touched

  // CV out //zero-initialize the config structure.
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  gpio_config(&io_conf);

  // serial
  uart_config_t uart_config = {
    .baud_rate = 31250, // midi speed
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
  };
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

  // link timer - phase
  SemaphoreHandle_t tickSemphr = xSemaphoreCreateBinary();
  timerGroup0Init(1000, tickSemphr);
  xTaskCreate(tickTask, "tick", 8192, tickSemphr, 1, nullptr);

  // midi clock
  const esp_timer_create_args_t periodic_timer_args = {
          .callback = &periodic_timer_callback,
          .name = "periodic"
  };
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 20833.333333333)); // 120 bpm by default
  
  //// LEDC //// PWM //// CV Pitch ////
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);
  ledc_fade_func_install(0);
  } 
} 
