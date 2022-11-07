static const char *MDNS_TAG = "mDNS";
static const char *SMART_TAG = "Smart config";
static const char *NVS_TAG = "NVS";
static const char *WIFI_TAG = "Wifi";

//// From mdns_example_main.c ////
static void initialise_mdns(void)
{
    char* hostname = strdup("tdlf-mdns");
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(MDNS_TAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set("tdlf-mDNS") );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"u","user"},
        {"p","password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add("tdlf", "_osc", "_udp", PORT, serviceTxtData, 3) );

    free(hostname);
}

/// SMART CONFIG /// WIFI STA ///
extern "C" { 
#define EXAMPLE_ESP_MAXIMUM_RETRY  2
char ssid[33]; 
char password[65];
static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when we are connected*/
#define WIFI_CONNECTED_BIT BIT0 // we are connected to the AP with an IP
#define WIFI_FAIL_BIT      BIT1  // we failed to connect after the maximum amount of retries 
static int s_retry_num = 0;
bool skipNVSRead = false;
}

bool goSMART = false;
bool goLINK = false;
static EventGroupHandle_t s_smartcfg_event_group; /* FreeRTOS event group to signal when smart config is done*/
TaskHandle_t xHandle;
static const int CONNECTED_BIT      = BIT0;  // est-ce nécessaire ?
static const int ESPTOUCH_DONE_BIT  = BIT1;  // depuis smart_config static const int ESPTOUCH_DONE_BIT = BIT1;

extern "C" {
  static void smartconfig_example_task(void * parm);
  } // depuis smart_config

extern "C" { 
  static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
  {
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START && goSMART == false) {
      ESP_LOGI(SMART_TAG,"Tente une connection avec les crédentials en mémoire");

      /// NVS READ CREDENTIALS ///
      nvs_handle wificfg_nvs_handler;
      size_t len;
      nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);

      nvs_get_str(wificfg_nvs_handler, "wifi_ssid", NULL, &len);
      char* lssid = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
      nvs_get_str(wificfg_nvs_handler, "wifi_ssid", lssid, &len);

      nvs_get_str(wificfg_nvs_handler, "wifi_password", NULL, &len);
      char* lpassword = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
      nvs_get_str(wificfg_nvs_handler, "wifi_password", lpassword, &len); // esp_err_t nvs_get_str(nvs_handle_thandle, const char *key, char *out_value, size_t *length)

      nvs_close(wificfg_nvs_handler);
   
      wifi_config_t wifi_config = { }; // when declaring wifi_config_t structure, do not forget to set all fields to zero.
     
      memcpy(wifi_config.sta.ssid, lssid, 33);
      memcpy(wifi_config.sta.password, lpassword, 65);

      ESP_LOGI(NVS_TAG,"wifi_config.sta.ssid NVS :%s",wifi_config.sta.ssid); 
      ESP_LOGI(NVS_TAG,"wifi_config.sta.ssid NVS :%s",wifi_config.sta.password); 

      ESP_ERROR_CHECK( esp_wifi_disconnect() );
      ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
      //ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

      esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == false) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            // goSMART = true; // reset smart config flag
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP failed");
        

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && goSMART == false) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        // ESP_LOGI(WIFI_TAG, "Got IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
	      esp_ip4addr_ntoa(&event->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	      ESP_LOGI(WIFI_TAG, "I have a connection and my IP is %s!", str_ip);  
      
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == true) {
        esp_wifi_connect();
        xEventGroupClearBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == true) {
        ESP_LOGI(SMART_TAG, "on a de quoi !");
        esp_wifi_connect();
        xEventGroupClearBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && goSMART == true) {
        ESP_LOGI(SMART_TAG, "on a de nouveau de quoi !");
        xEventGroupSetBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE && goSMART == true) {
        ESP_LOGI(SMART_TAG, "Scan done");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL && goSMART == true) {
        ESP_LOGI(SMART_TAG, "Found channel");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD && goSMART == true) {

      ESP_LOGI(SMART_TAG, "Got SSID and password");

      smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

      wifi_config_t wifi_config;
      bzero(&wifi_config, sizeof(wifi_config_t)); // or wifi_config_t wifi_config = { }; // when declaring wifi_config_t structure, do not forget to set all fields to zero.
      
      memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
      memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
      
      wifi_config.sta.bssid_set = evt->bssid_set;

      if (wifi_config.sta.bssid_set == true) {
        ESP_LOGI(SMART_TAG, "bssid_set is true so normally we copy the credentials in memory");
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

      memcpy(ssid, evt->ssid, sizeof(evt->ssid)); 
      memcpy(password, evt->password, sizeof(evt->password));

      ESP_LOGI(SMART_TAG, "MEMCPY SSID:%s", ssid);
      ESP_LOGI(SMART_TAG, "MEMCPY PASSWORD:%s", password);

      //////// WRITING TO NVS //////
      nvs_handle wificfg_nvs_handler;
      nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_set_str(wificfg_nvs_handler,"wifi_ssid",ssid);
      nvs_set_str(wificfg_nvs_handler,"wifi_password",password);
      nvs_commit(wificfg_nvs_handler); 
      nvs_close(wificfg_nvs_handler); 
      ////// END NVS ///// 

      /*///// TEST READ FROM NVS /////
      size_t len;
      nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_get_str(wificfg_nvs_handler, "wifi_ssid", NULL, &len);
      char* ssidtest = (char*)malloc(len);
      nvs_get_str(wificfg_nvs_handler,"wifi_ssid", ssidtest, &len);
      nvs_close(wificfg_nvs_handler); 
      ESP_LOGI(NVS_TAG,"TEST READ NVS SSID:%s",ssidtest);
      ////////////////////*/

      ESP_ERROR_CHECK( esp_wifi_disconnect() );
      ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
      ESP_ERROR_CHECK( esp_wifi_connect() );

    } // end of writing ssid + password to nvs from smartcfg
    
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        ESP_LOGI(SMART_TAG, "ESPTOUCH DONE!");
        xEventGroupSetBits(s_smartcfg_event_group, ESPTOUCH_DONE_BIT);
    }
  } // fin event handler
 
} // fin extern "C"

extern "C" { static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t smtcfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&smtcfg) );
    ESP_LOGI(SMART_TAG,"normalement on a démarré le smartconfig");

    while (1) {

      vTaskDelay(500 / portTICK_PERIOD_MS); // 35000 // 15000 // 5000 besoin d'un long délai ça l'air
      uxBits = xEventGroupWaitBits(s_smartcfg_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        
      if(uxBits & CONNECTED_BIT) {
          ESP_LOGI(SMART_TAG, "WiFi Connected to ap");
        }

      if(uxBits & ESPTOUCH_DONE_BIT) {
        ESP_LOGI(SMART_TAG, "smartconfig over");
        goLINK = true;
        esp_smartconfig_stop();
        vTaskDelete( xHandle ); // tente de fermer le task correctement
        //vTaskGetRunTimeStats( xHandle );
      }
    }
  } 

} // fin extern "C"

extern "C" { void wifi_init_sta(void)
{
  if ( DefaultBusInit( ) == true ) {
    printf( "BUS Init lookin good\n" );
    SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Technologies", SSD_COLOR_WHITE );
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "de la fete", SSD_COLOR_WHITE );
    SSD1306_SetVFlip( &I2CDisplay, 1 ); 
    SSD1306_SetHFlip( &I2CDisplay, 1 ); //void SSD1306_SetHFlip( struct SSD1306_Device* DeviceHandle, bool On );
    SSD1306_Update( &I2CDisplay );  
   }

  s_wifi_event_group = xEventGroupCreate();
  s_smartcfg_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
  ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

  /// NVS READ CREDENTIALS ///
  nvs_handle wificfg_nvs_handler;
  size_t len;
  nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);

  nvs_get_str(wificfg_nvs_handler, "wifi_ssid", NULL, &len);
  char* lssid = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
  nvs_get_str(wificfg_nvs_handler, "wifi_ssid", lssid, &len);

  nvs_get_str(wificfg_nvs_handler, "wifi_password", NULL, &len);
  char* lpassword = (char*)malloc(len); // (char*)malloc(len) compiles but crashes
  nvs_get_str(wificfg_nvs_handler, "wifi_password", lpassword, &len); // esp_err_tnvs_get_str(nvs_handle_thandle, const char *key, char *out_value, size_t *length)

  nvs_close(wificfg_nvs_handler);
   
  ESP_LOGI(NVS_TAG,"WIFI_STA ssid :%s",lssid); 
  ESP_LOGI(NVS_TAG,"WIFI_STA password :%s",lpassword); 

  wifi_config_t wifi_config;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start() );

  ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

  // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
  // number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) 

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY);

  // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
  // happened. 
  if (bits & WIFI_CONNECTED_BIT) {
    // ESP_LOGI(WIFI_TAG, "Connected to WiFI");
    SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
    SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, str_ip, SSD_COLOR_WHITE ); 
    SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24 );
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "Link!", SSD_COLOR_WHITE );
    SSD1306_Update( &I2CDisplay );   
    } 
    else if (bits & WIFI_FAIL_BIT) {
      ESP_LOGE(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
      wifi_config.sta.ssid, wifi_config.sta.password);
      goSMART = true; // pour la suite des choses
      SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
      SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
      SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Use", SSD_COLOR_WHITE );
      SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "ESPTouch", SSD_COLOR_WHITE );
      SSD1306_Update( &I2CDisplay );
      } 
    else {
      ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }
    //The event will not be processed after unregister 
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //vEventGroupDelete(s_wifi_event_group);
  } 
} // fin extern "C" */