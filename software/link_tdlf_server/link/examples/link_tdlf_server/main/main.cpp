////// tdlf server ///// receive note, channel info through UDP + send midi through serial to instruments
////// ESP32 Ableton Link node // midi clock // BPM (+ - )// Start/Stop
////// Smart config NVS enabled to set the wifi credentials from ESPTouch app if no IP is attributed


#include "mstrpck.h"
#include <ableton/Link.hpp>
#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <protocol_examples_common.h>
#include "driver/uart.h"
#include <stdio.h>
#include "esp_timer.h"
#include "esp_sleep.h"

#include <chrono> // essai pour setTempo()

extern "C" {
#include <string.h> // De l'exemple station_example_main
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <stdbool.h> 
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h" // inet.pton() ??

#include <stdlib.h> // De l'exemple smart_config
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "freertos/queue.h"
#include "esp_log.h"
}

#define USE_TOUCH_PADS // touch_pad_5 (GPIO_NUM_12), touch_pad_7 (GPIO_NUM_27), touch_pad_9 (GPIO_NUM_32)
#define USE_I2C_DISPLAY // SDA GPIO_NUM_25 (D2), SCL GPIO_NUM_33 (D1)
#define USE_SOCKETS // we receive data from the seq clients

extern "C" {
static const char *SOCKET_TAG = "Socket";
static const char *TOUCH_TAG = "Touch pad";
}

/////// sockette server ///////
#if defined USE_SOCKETS
  extern "C" {
  #include "lwip/err.h" // udp_server example
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
  #include <lwip/netdb.h>
  }
  #define PORT 3333

#endif
////// sockette server //////

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE


// Serial midi
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_TXD  (GPIO_NUM_13) // TTGO pin 4 // 13 ?
#define ECHO_TEST_RXD  (GPIO_NUM_5)
#define BUF_SIZE (1024)
#define MIDI_TIMING_CLOCK 0xF8
#define MIDI_START 0xFA // 11111010 // 250
#define MIDI_STOP 0xFC // 11111100 // 252

char MIDI_NOTE_ON_CH[] = {0x99,0x90}; // note on, channel 10, note on, channel 0 // ajouter d'autres séries
// char zeDrums[] = {0x24,0x26,0x2B,0x32,0x2A,0x2E,0x27,0x4B,0x43,0x31}; // midi drum notes in hexadecimal format
// char zeDark[] = {0x3D,0x3F,0x40,0x41,0x42,0x44,0x46,0x47}; // A#(70)(0x46), B(71)(0x47), C#(61)(0x3D), D#(63)(0x3F), E(64)(0x40), F(65)(0x41), F#(66)(0x42), G#(68)(0x44)

// octave : C |C# D |D# E F |F# G|G# A|A# B 

/*
0x24 // 36 // C2 //  Kick
0x26 // 38 // D2 //  Snare
0x2B // 43 // G2 //  Low tom
0x32 // 50 // D3 //  Hi Tom 
0x2A // 42 // F#2 // Closed Hat 
0x2E // 46 // A#2 // Open Hat
0x27 // 39 // D#2 // Clap 
0x4B // 75 // D#5 // Claves 
0x43 // 67 // G4 //  Agogo 
0x31 // 49 // C#3 // Crash
*/

// penser à un système qui réarrange les notes selon les gammes, on a huit notes...
// est-ce que l'utilisateur peut transposer d'octave...sans doute

#define MIDI_NOTE_VEL 0x64 // 1100100 // 100 // note on,  // data
#define MIDI_SONG_POSITION_POINTER 0xF2

///// seq /////

bool mstr[75] = {}; // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-9] (bar) // mstr[10] (mute) // mstr[11-74](steps)
int channel; // 4 bits midi channel (0-7) -> (10,1,2,3,4,5,6,7) // drums + más
int note; // 4 bits note info // 8 notes correspond to 8 colors // (0-7) -> (36,38,43,50,42,46,39,75),67,49 // más de 8 !
int bar[8] = {1,1,1,1,1,1,1,1}; // 2 bits, up to 4 bars?
bool muteRecords[8] = {0,0,0,0,0,0,0,0}; // mute info per
int stepsLength[8] = {16,16,16,16,16,16,16,16}; // varies per note 16-64

int mtmstr[16][64]; // note // beats 

int beat = 0; 
int step = 0 ;
int dubStep = 0;

float oldstep;

static void periodic_timer_callback(void* arg);
esp_timer_handle_t periodic_timer;

bool startStopCB = false; // l'état du callback 
bool startStopState = false; // l'état local

bool isPlaying = true;

bool changePiton = false; 
bool changeLink = false;
bool tempoINC = false; // si le tempo doit être augmenté
bool tempoDEC = false; // si le tempo doit être réduit
double newBPM; // pour tenter d'envoyer à setTempo();
double curr_beat_time;
double prev_beat_time;

bool connektMode = true; // flag pour envoyer l'adresse IP aux clients

char str_ip[16] ="192.168.0.42"; // send IP to clients !! // stand in ip necessary for memory space?

/// smart config /// wifi_sta ///
#define EXAMPLE_ESP_MAXIMUM_RETRY  2
char ssid[33]; // ISO C++ forbids converting a string constant to 'char*'
char password[65];


/////////////////// I2C Display //////////////////
#if defined USE_I2C_DISPLAY
extern "C" {
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

char buf[20]; // BPM display
char compte[8];
char current_phase_step[4];

static const int I2CDisplayAddress = 0x3C;
static const int I2CDisplayWidth = 128; // wemos oled screen width and height
static const int I2CDisplayHeight = 64; 
static const int I2CResetPin = -1;

struct SSD1306_Device I2CDisplay;

bool DefaultBusInit( void ) {  
        assert( SSD1306_I2CMasterInitDefault( ) == true );
        assert( SSD1306_I2CMasterAttachDisplayDefault( &I2CDisplay, I2CDisplayWidth, I2CDisplayHeight, I2CDisplayAddress, I2CResetPin ) == true );
    return true;
}

void FontDisplayTask( void* Param ) {
    struct SSD1306_Device* Display = ( struct SSD1306_Device* ) Param;

    if ( Param != NULL ) {

        SSD1306_SetFont( Display, &Font_droid_sans_mono_13x24);
        SSD1306_Clear( Display, SSD_COLOR_BLACK );
        SSD1306_FontDrawAnchoredString( Display, TextAnchor_North, "BPM", SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( Display, TextAnchor_Center, "66.6", SSD_COLOR_WHITE );
        SSD1306_Update( Display );
    }

    vTaskDelete( NULL );
}
    
void SetupDemo( struct SSD1306_Device* DisplayHandle, const struct SSD1306_FontDef* Font ) {
    SSD1306_Clear( DisplayHandle, SSD_COLOR_BLACK );
    SSD1306_SetFont( DisplayHandle, Font );
}

void SayHello( struct SSD1306_Device* DisplayHandle, const char* HelloText ) {
    SSD1306_FontDrawAnchoredString( DisplayHandle, TextAnchor_Center, HelloText, SSD_COLOR_WHITE );
    SSD1306_Update( DisplayHandle );
}
     
} 
#endif ////////////// END I2C Display init /////////////////////////


#if defined USE_TOUCH_PADS
extern "C" {
#include "driver/touch_pad.h" 
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

//#define TOUCH_PAD_NO_CHANGE   (-1) // not necessary ?
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80) // 95
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10) // 10

static bool s_pad_activated[16];
static uint32_t s_pad_init_val[16];

static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;
     for (int i = 5; i < 10; i=i+2) {
        touch_pad_read_filtered((touch_pad_t)i, &touch_value);
        s_pad_init_val[i] = touch_value;
        ESP_LOGI(TOUCH_TAG, "test init: touch pad [%d] val is %d", i, touch_value); //set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)i, touch_value * 2 / 3));
     }
}
static void tp_example_read_task(void *pvParameter) {
    
    
 while (1) {
     
    touch_pad_intr_enable();
    for (int i = 5; i < 10; i=i+2) {
        if (s_pad_activated[5] == true) {
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 5);  // Wait a while for the pad being released
        tempoINC = true; // pour que le audio loop le prenne en compte
        vTaskDelay(300 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[5] = false; // Reset the counter triggering a message // that application is running

        } else if (s_pad_activated[7] == true) {
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 7);  
        tempoDEC = true; 
        vTaskDelay(300 / portTICK_PERIOD_MS);  
        s_pad_activated[7] = false; 

        } else if (s_pad_activated[9] == true) {
        ESP_LOGI(TOUCH_TAG, "T%d piton!", 9);  
        startStopState = !startStopState; 
        changePiton = true;
        ESP_LOGI(TOUCH_TAG, "startStopState : %i ", startStopState);
        ESP_LOGI(TOUCH_TAG, "changePiton : %i ", changePiton);
        vTaskDelay(300 / portTICK_PERIOD_MS);  
        s_pad_activated[9] = false;  
        }
    }
        
    vTaskDelay(10 / portTICK_PERIOD_MS);
     
    }
}


static void tp_example_rtc_intr(void *arg) { //  Handle an interrupt triggered when a pad is touched. Recognize what pad has been touched and save it in a table.
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status(); //clear interrupt
    for (int i = 5; i < 10; i = i+2) {
    //for (int i = 7; i < 8; i++) { // juste touch 7
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}


static void tp_example_touch_pad_init(void) { // Before reading touch pad, we need to initialize the RTC IO.
    for (int i = 5; i < 10; i = i+2) {   
        touch_pad_config((touch_pad_t)i, TOUCH_THRESH_NO_USE); //init RTC IO and mode for touch pad.
    }
}

}    
#endif

////////////////////// sockette server ///////////////////////
#if defined USE_SOCKETS
extern "C" {
  static void udp_server_task(void *pvParameters)
  {
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) { 

        struct sockaddr_in6 dest_addr; // IPV6*/
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

        if (sock < 0) {
            ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err < 0) {
            ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
        }

        ESP_LOGI(SOCKET_TAG, "Socket bound, port %d", PORT);

        if (err < 0) {
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        

        while (1) {

            ESP_LOGI(SOCKET_TAG, "Waiting for data");

            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            
            //mstr devrait être 75 valeurs;
            int len = recvfrom(sock, mstr, sizeof(mstr), 0, (struct sockaddr *)&source_addr, &socklen);
           
            //for (int i = 0; i < sizeof(mstr);i++){
            //    ESP_LOGE(SOCKET_TAG, "mstr %i :%i", i, mstr[i]);
            //}

            

            // Filter the array input and populate mtmstr

            int tmpTotal = 0; // reset before counting

            for(int i=0;i<4;i++){ // 

              if(i==3 && mstr[3+4] == true){
                tmpTotal = tmpTotal+1;
                }
              else if(i==2 && mstr[2+4] == true){
                tmpTotal = tmpTotal + 2;
                }
              else if(i==1 && mstr[1+4] == true){
                tmpTotal = tmpTotal + 4;
                }  
            }

            note = tmpTotal; // only 8 note values for the moment

            ESP_LOGI(SOCKET_TAG, "note : %i", note); 

            // read in the bit value for mute and store 
            muteRecords[note] = mstr[10]; 
            muteRecords[note] = true; 

            // read in bar value from mst[8] and mst[9] and save it as int for the corresponding note
            if(mstr[8]==false && mstr[9]==false){bar[note] = 1;} 
            else if(mstr[8]==true && mstr[9]==false){bar[note] = 2;} 
            else if(mstr[8]==false && mstr[9]==true){bar[note] = 3;} 
            else {bar[note] = 4;} // true && true 

            ESP_LOGI(SOCKET_TAG, "bar[note] : %i", bar[note]); 
            
            for( int i=0; i<8; i++ ){
            
              if ( i == note ){ // write into the array at the correct note index
                  for( int j=0; j<64; j++ ) {
                    if(mstr[j+11]){ // 11 bit offset from the array
                      mtmstr[note][j] = 1; 
                    }
                    else {
                      mtmstr[note][j] = 0; 
                    }
                  }                   
              }
            }


            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(SOCKET_TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            
            else {   // Data received
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1); // Get the sender's ip address as string
                }
                
                ESP_LOGI(SOCKET_TAG, "Received %d bytes from %s:", len, addr_str);
              
                ESP_LOGI(SOCKET_TAG, "Sent my IP %s", str_ip); 
                int err = sendto(sock, str_ip, sizeof(str_ip), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));

                if (err < 0) {
                    ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
       
        }

        if (sock != -1) {
            ESP_LOGI(SOCKET_TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
  }
}
#endif
////////// fin sockette server //////////////


/////////////////// WiFI smart station config //////////////////////

bool goSMART = false;
bool goLINK = false;
static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_smartcfg_event_group; /* FreeRTOS event group to signal when smart config is done*/
TaskHandle_t xHandle;

#define WIFI_CONNECTED_BIT BIT0 // we are connected to the AP with an IP
#define WIFI_FAIL_BIT      BIT1  // we failed to connect after the maximum amount of retries 

static const int CONNECTED_BIT      = BIT0;  // est-ce nécessaire ?
static const int ESPTOUCH_DONE_BIT  = BIT1;  // depuis smart_config static const int ESPTOUCH_DONE_BIT = BIT1;

static const char *TAG = "smart wifi station";
static int s_retry_num = 0;

extern "C" {
  static void smartconfig_example_task(void * parm);
  } // depuis smart_config

extern "C" { 
  static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
  {
    // station_example
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START && goSMART == false) {
        ESP_LOGI(TAG,"Tente une connection avec les crédentials en mémoire");

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

    //ESP_LOGI(TAG,"wifi_config.sta.ssid NVS :%s",wifi_config.sta.ssid); 
    //ESP_LOGI(TAG,"wifi_config.sta.ssid NVS :%s",wifi_config.sta.password); 

    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == false) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            // goSMART = true; // reset smart config flag
        }
        ESP_LOGI(TAG,"connect to the AP failed....");
        

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && goSMART == false) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(TAG, "got ip: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
  
	      esp_ip4addr_ntoa(&event->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	      ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);  
        
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == true) {
        esp_wifi_connect();
        xEventGroupClearBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == true) {
        ESP_LOGI(TAG, "on a de quoi !");
        esp_wifi_connect();
        xEventGroupClearBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && goSMART == true) {
        ESP_LOGI(TAG, "on a de nouveau de quoi !");
        xEventGroupSetBits(s_smartcfg_event_group, CONNECTED_BIT);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE && goSMART == true) {
        ESP_LOGI(TAG, "Le Scan done");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL && goSMART == true) {
        ESP_LOGI(TAG, "Found channel");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD && goSMART == true) {

      ESP_LOGI(TAG, "Got SSID and password");

      smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

      wifi_config_t wifi_config;
      bzero(&wifi_config, sizeof(wifi_config_t)); // ... or wifi_config_t wifi_config = { }; // when declaring wifi_config_t structure, do not forget to set all fields to zero.
      
      memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
      memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
      
      wifi_config.sta.bssid_set = evt->bssid_set;

      if (wifi_config.sta.bssid_set == true) {
        ESP_LOGI(TAG, "bssid_set is true so normally we copy the credentials in memory");
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

      memcpy(ssid, evt->ssid, sizeof(evt->ssid)); 
      memcpy(password, evt->password, sizeof(evt->password));

      ESP_LOGI(TAG, "MEMCPY SSID:%s", ssid);
      ESP_LOGI(TAG, "MEMCPY PASSWORD:%s", password);

      //////// WRITING TO NVS // EVENTUALLY USE THIS TO SAVE mtmstr[] //////
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
      ESP_LOGI(TAG,"TEST READ NVS SSID:%s",ssidtest);
      ////////////////////*/

      ESP_ERROR_CHECK( esp_wifi_disconnect() );
      ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
      ESP_ERROR_CHECK( esp_wifi_connect() );

    } // end of writing ssid + password to nvs from smartcfg
    
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        ESP_LOGI(TAG, "ESPTOUCH DONE!");
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
    ESP_LOGI(TAG,"normalement on a démarré le smartconfig");
    while (1) {
      ESP_LOGI(TAG,"et puis là : ...");

      #if defined USE_I2C_DISPLAY   
        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13);
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Use", SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "ESPTouch", SSD_COLOR_WHITE );
      #endif 

      vTaskDelay(35000 / portTICK_PERIOD_MS); // besoin d'un long délai ça l'air
      uxBits = xEventGroupWaitBits(s_smartcfg_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        
      if(uxBits & CONNECTED_BIT) {
          ESP_LOGI(TAG, "WiFi Connected to ap");
        }

      if(uxBits & ESPTOUCH_DONE_BIT) {
        ESP_LOGI(TAG, "smartconfig over");
        goLINK = true;
        esp_smartconfig_stop();
        ESP_LOGI(TAG,"this will print ok ");
        vTaskDelete( xHandle ); // tente de fermer le task correctement
        // ESP_LOGI(TAG,"this will not print as we exit beforehand");
        //vTaskGetRunTimeStats( xHandle );
      }
    
  }
} // fin extern "C"

}

extern "C" { void wifi_init_sta(void)
{

  #if defined USE_I2C_DISPLAY   
        if ( DefaultBusInit( ) == true ) {
        printf( "BUS Init lookin good...\n" );
       
        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13);
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Technologies", SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "de la fete", SSD_COLOR_WHITE );
        SSD1306_Update( &I2CDisplay );  

   }
   #endif

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
   
    ESP_LOGI(TAG,"WIFI_STA NVS :%s",lssid); 
    ESP_LOGI(TAG,"WIFI_STA NVS :%s",lpassword); 

    wifi_config_t wifi_config;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "nous sommes connectés par WiFI");
        
        #if defined USE_I2C_DISPLAY   
        SetupDemo( &I2CDisplay, &Font_droid_sans_mono_13x24 );
        SayHello( &I2CDisplay, "Link!" );
        #endif 
        
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
                 goSMART = true; // pour la suite des choses
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
   // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
   // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //vEventGroupDelete(s_wifi_event_group);
  } // fin extern "C"
}


unsigned int if_nametoindex(const char* ifName)
{
  return 0;
}

char* if_indextoname(unsigned int ifIndex, char* ifName)
{
  return nullptr;
}

void IRAM_ATTR timer_group0_isr(void* userParam)
{
  static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  TIMERG0.int_clr_timers.t0 = 1;
  TIMERG0.hw_timer[0].config.alarm_en = 1;

  xSemaphoreGiveFromISR(userParam, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

void timerGroup0Init(int timerPeriodUS, void* userParam)
{
  timer_config_t config = {.alarm_en = TIMER_ALARM_EN,
    .counter_en = TIMER_PAUSE,
    .intr_type = TIMER_INTR_LEVEL,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = 80};

  timer_init(TIMER_GROUP_0, TIMER_0, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timerPeriodUS);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_group0_isr, userParam, 0, nullptr);

  timer_start(TIMER_GROUP_0, TIMER_0);
}


// callbacks
void tempoChanged(double tempo) {
    ESP_LOGI(TAG, "tempochanged");
    double midiClockMicroSecond = ((60000 / tempo) / 24) * 1000;

#if defined USE_I2C_DISPLAY
    char buf[10];
    snprintf(buf, 10 , "%i", (int) round( tempo ) );
    SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
    SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24);
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "BPM", SSD_COLOR_WHITE );
    SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, buf, SSD_COLOR_WHITE );
    SSD1306_Update( &I2CDisplay );   
#endif

    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) periodic_timer;
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, midiClockMicroSecond));
}

void startStopChanged(bool state) {
  // received as soon as sent, we can get the state of 'isPlaying' and use that
  // need to wait for phase to be 0 (and deal with latency...)
  startStopCB = state;
  changeLink = true;

  //ESP_LOGI(TAG, "startstopCB is : %i", state);
  //ESP_LOGI(TAG, "changeLink is : %i", state);
  
}


extern "C" {

  void convertBits2Int(int sel){
    //ESP_LOGI(TAG, "conversion");  
    channel = 0; // reset avant de recompter
    // note = 0; // note is calculated when the array is received
    int tmpTotal = 0;

    for(int i=0;i<4;i++){ // up to 8 channels, could hold 16 values

      if(i==3 && mstr[3+4*sel] == true){
        tmpTotal = tmpTotal+1;
      }
      else if(i==2 && mstr[2+4*sel] == true){
        tmpTotal = tmpTotal + 2;
      }
      else if(i==1 && mstr[1+4*sel] == true){
        tmpTotal = tmpTotal + 4;
      }
    }
      // ESP_LOGI(TAG, "channel : %i", tmpTotal); 
      channel = tmpTotal;
  }

}

void tickTask(void* userParam)
{
  // connect link
  ableton::Link link(120.0f);
  link.enable(true);
  link.enableStartStopSync(true); // if not no callback for start/stop

  // callbacks
  link.setTempoCallback(tempoChanged);
  link.setStartStopCallback(startStopChanged);

  // phase
  while (true)
  { 
    xSemaphoreTake(userParam, portMAX_DELAY);

    const auto state = link.captureAudioSessionState(); 
    isPlaying = state.isPlaying();
    //  ESP_LOGI(TAG, "isPlaying : , %i", isPlaying);  
    //const auto phase = state.phaseAtTime(link.clock().micros(), 1); 
    //ESP_LOGI(TAG, "tempoINC : %i", tempoINC);
    //ESP_LOGI(TAG, "tempoDEC : %i", tempoDEC);

    if ( tempoINC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo + 1;
      ESP_LOGI(TAG, "BPM changed %i", int(newBPM));
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession); // le problème est que l'instruction de changer le tempo nous revient
      tempoINC = false;
      //ESP_LOGI(TAG, "tempoINC : %i", tempoINC);
    }

    if ( tempoDEC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo - 1;
      ESP_LOGI(TAG, "BPM changed %i", int(newBPM));
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession);
      tempoDEC = false;
    }

    ////////// test start stop send to other clients /////////
    //ESP_LOGI(TAG, "PRE startStopState is :  %i", startStopState);  
    //ESP_LOGI(TAG, "PRE startStopCB is :  %i", startStopCB); 

    if ( changePiton && startStopState != startStopCB ){ // if local state is different to the SessionState then send to the session state 
        auto mySession = link.captureAppSessionState();
        const auto timez = link.clock().micros();
        mySession.setIsPlaying(startStopState, timez);
        link.commitAppSessionState(mySession);
      }
      
    if ( changeLink && startStopState != startStopCB ){ // if CB state is different to the local startStopState then resync the latter to the former
       startStopState = startStopCB; // resync 
      }

    //ESP_LOGI(TAG, "POST startStopState is :  %i", startStopState);  
    //ESP_LOGI(TAG, "POST startStopCB is :  %i", startStopCB); 
   
    curr_beat_time = state.beatAtTime(link.clock().micros(), 4);
    const double curr_phase = fmod(curr_beat_time, 4);  // const double

    if (curr_beat_time > prev_beat_time ) {
      
      const double prev_phase = fmod(prev_beat_time, 4);
      const double prev_step = floor(prev_phase * 4);
      const double curr_step = floor(curr_phase * 4);
      
      //ESP_LOGI(TAG, "current step : %f", curr_step); 
      //ESP_LOGI(TAG, "prev step : %f", prev_step); 
     
      if (abs(prev_phase - curr_phase) > 4 / 2 || prev_step != curr_step) { // quantum divisé par 2

        if( ( curr_step == 0 && changePiton ) || ( curr_step == 0 && changeLink ) ) { // start / stop implementation
            
              if(startStopCB) { // saw a simpler way of doing this!
                char zedata[] = { MIDI_START };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                uart_write_bytes(UART_NUM_1, 0, 1);
                changeLink = false;
                changePiton = false;
                step = 0; // reset step count
              } 
              else { // on jouait et on arrête // changer pout s'arrêter immédiatement après un stop 
                char zedata[] = { MIDI_STOP };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                uart_write_bytes(UART_NUM_1, 0, 1);
                changeLink = false;
                changePiton = false;
                }
            }
  

        if (step == 64){ // max steps = 64
          step = 0;
          }

        if(startStopCB){
          SSD1306_SetInverted( &I2CDisplay, 1);
          }
          
        if(!startStopCB){
          SSD1306_SetInverted( &I2CDisplay, 0);  
          }

        const int halo_welt = int(curr_phase);

        const int tmpOH = (int) round( state.tempo() );
        // ESP_LOGI(TAG, "tempo %i", tmpOH); 

        char tmpOHbuf[20];

        snprintf(tmpOHbuf, 20 , "%i", tmpOH );     /////// display BPM + Phase + Step /////////
        snprintf(current_phase_step, 8, "   %i %i", halo_welt, step);
        SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24); // &Font_droid_sans_fallback_15x17
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, tmpOHbuf, SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_West, current_phase_step, SSD_COLOR_WHITE );
        SSD1306_Update( &I2CDisplay );  

        if (startStopCB){ // isPlaying and did we send that note out? 

          convertBits2Int(0); // get current channel // drums or synths ?

          // passe ds ttes les notes de mtmstr[] et sors ça...assez vite j'espère.
          for(int i = 0; i<8;i++){ // faut faire ça pour chaque valeur de note
                
              // compteur de quel bar on est rendus pour les séquences plus longues...
              stepsLength[i] = bar[i]*16; // correspond au nombre de bars pour la note
              //ESP_LOGI(TAG, "stepsLength[i] : %i", stepsLength[i]);
              dubStep = step%stepsLength[i]; // modulo 16 // ... 32 // ... 48 // ... 64
              //ESP_LOGI(TAG, "nouveau dubStep : %i", dubStep);

              //if (mtmstr[i][step] == 1 && !muteRecords[i]){ // send midi note out 
              //ESP_LOGI(TAG, "mtmstr[i][step] : %i", i);
              if (mtmstr[i][dubStep] == 1){ // send midi note out // mute to be implemented
                
                if (channel == 0){ // are we playing drumz ?
                  //ESP_LOGI(TAG, "drums : %i", i);
                  char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme 10 pour l'instant mais dois pouvoir changer
                  uart_write_bytes(UART_NUM_1, zedata1, 1); // this function will return after copying all the data to tx ring buffer, UART ISR will then move data from the ring buffer to TX FIFO gradually.
                  char zedata2[] = {zeDrums[i]};      
                  uart_write_bytes(UART_NUM_1, zedata2, 1); // tableau de valeurs de notes hexadécimales 
                }
                else if (channel == 1){ // premier jeu de notes
                  char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme midi channel channel 0 
                  uart_write_bytes(UART_NUM_1, zedata1, 1);                 
                  char zedata2[] = {zeDark[i]}; // tableau de valeurs de notes hexadécimales 
                  uart_write_bytes(UART_NUM_1, zedata2, 1); 
                }
                else{} // ajouter d'autres gammes (scales)
                
                char zedata3[] = { MIDI_NOTE_VEL };
                uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                // uart_write_bytes(UART_NUM_1, 0, 1); // ??

              }
                  
          }
        }

        if(isPlaying){
          // ESP_LOGI(TAG, "step : %d", step); 
          step++; // might be off to add '1' right away
          }

      }

    } // fin de if curr_beat_time is > prev_beat_time

    prev_beat_time = curr_beat_time;

    portYIELD();
  }

} // fin de tick task

static void periodic_timer_callback(void* arg)
{
    char zedata[] = { MIDI_TIMING_CLOCK };
    //ESP_LOGI(TAG, "MIDI_TIMING_CLOCK");
    uart_write_bytes(UART_NUM_1, zedata, 1);
}


extern "C" { void app_main()
{ 
  //Initialize and read in wifi credentials from NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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
    
    ESP_LOGI(TAG,"INIT NVS :%s",lssid); 
    ESP_LOGI(TAG,"INIT NVS :%s",lpassword); 


    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    if (goSMART == true){
        //ESP_ERROR_CHECK(esp_wifi_stop());
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, "STOP STA");
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, &xHandle);
    }

    ESP_LOGI(TAG, "weeeeeiiiiiiird ##################");
    tcpip_adapter_init();
  
  //ESP_ERROR_CHECK(esp_event_loop_create_default());
  

  #if defined USE_SOCKETS // yep sockette server //
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
  #endif


  #if defined USE_TOUCH_PADS
    ESP_LOGI(TOUCH_TAG, "Initializing touch pad");     // Initialize touch pad peripheral, it will start a timer to run a filter
    touch_pad_init();
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);   // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);// Set reference voltage for charging/discharging
    tp_example_touch_pad_init();    // Init touch pad IO
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD); // Initialize and start a software filter to detect slight change of capacitance.
    tp_example_set_thresholds();     // Set thresh hold
    touch_pad_isr_register(tp_example_rtc_intr, NULL); // Register touch interrupt ISR
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL); // Start a task to show what pads have been touched
  #endif

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
  
  //} // fin de goLINK


  } // fin de extern "C"
} // fin de main()
