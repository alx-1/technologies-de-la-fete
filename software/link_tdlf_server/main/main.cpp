////// tdlf server ///// receive note, midi channel info through UDP + send midi through serial // Save, Load sequences
////// ESP32 Ableton Link node // midi clock // BPM (+ - )// Start/Stop 
////// Smart config NVS enabled to set the wifi credentials from ESPTouch app if no IP is attributed

#include "mstrpck.h"
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
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h" // inet.pton() 

#include <stdlib.h> // from the smart_config example
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "driver/ledc.h" // For CV Pitch 
#include "esp_err.h"

///// MDNS //////
#include "mdns.h"//
#include "netdb.h" //

///// OSC ///////
#include <tinyosc.h> // still to test for speed !!

}

#define USE_TOUCH_PADS // touch_pad_2 (GPIO_NUM_2), touch_pad_3 (GPIO_NUM_15), touch_pad_4 (GPIO_NUM_14), touch_pad_5 (GPIO_NUM_12), touch_pad_7 (GPIO_NUM_27), touch_pad_9 (GPIO_NUM_32)
#define USE_I2C_DISPLAY // SDA GPIO_NUM_25 (D2), SCL GPIO_NUM_33 (D1) // or SDA 21 SCL 22 // set values in menuconfig!
#define USE_SOCKETS // we receive data from the seq clients

extern "C" {
  static const char *SOCKET_TAG = "Socket";
  static const char *TOUCH_TAG = "Touch pad";
  static const char *SMART_TAG = "Smart config";
  static const char *NVS_TAG = "NVS";
  static const char *WIFI_TAG = "Wifi";
  static const char *LINK_TAG = "Link";
  static const char *TAP_TAG = "Tap";
  static const char *MIDI_TAG = "Midi";
  static const char *CV_TAG = "CV";
  static const char *MDNS_TAG = "mDNS";
  static const char *SEQ_TAG = "Sequence";
}

/////// sockette server ///////
#if defined USE_SOCKETS
  extern "C" {
  #include "lwip/err.h" // udp_server example
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
  #include <lwip/netdb.h>

  char addr_str[128]; // Copié depuis le client
  int addr_family;    
  int ip_protocol;
  int sock_WT32;
  struct sockaddr_in dest_addr_WT32;
  #define HOST_IP_ADDR_WT32 "192.168.50.240" // Hard-coded to send data to MaxD //
  #define PORT_WT32 3333
  }
  
  #define PORT 3333

  char clientIPAddresses[8][21]; // 8 potential clients, IPv6 format + 1 for string termination by strncat
  
  int clientIPCheck ( char myArray[] ) { // ESP_LOGI(SOCKET_TAG, "This is myArray : %s", myArray);
    
    for ( int i = 0; i < 8; i++ ) { // max of 8 IP addresses connected
      if( strcmp( myArray, clientIPAddresses[i] ) == 0 ) {
        return i; // IP Address already exists at position 'i' in the array
        break; 
        }

    }
    return 42; // not in the array, add it
  }
#endif
////// sockette server //////

// CV Outputs //
#define CV_18  (GPIO_NUM_18) 
#define CV_5  (GPIO_NUM_5) 
#define GPIO_OUTPUT_PIN_SEL  ( (1ULL<<CV_18) | (1ULL<<CV_5) )
int CV_TIMING_CLOCK = 0; 

// Serial midi
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_TXD  (GPIO_NUM_26) // was TTGO touch  pin 4 (GPIO_NUM_17) // 13 // change for GPIO_NUM_17 // GPIO_NUM_26
#define ECHO_TEST_RXD  (GPIO_NUM_19) 
#define BUF_SIZE (1024)
#define MIDI_TIMING_CLOCK 0xF8
#define MIDI_NOTE_OFF 0x80 // 10000000 // 128
#define MIDI_START 0xFA // 11111010 // 250
#define MIDI_STOP 0xFC // 11111100 // 252

char MIDI_NOTE_ON_CH[] = {0x90,0x91,0x92,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F}; // note on, channel 10, note on, channel 0 // ajouter d'autres séries

//////// CC messages config //////////
// Load MIDI CC config from NVS if it exists and turn this off is a cfg exists //
// We might not need to do much config of CC messages as the sensor sends it's own Channel and CC values
bool need2configCC = true; // Keep an eye out for sensor messages, if so, start the config of midi CC messages
bool configCC = false; // maybe only one of these two is needed !
bool configCCChannel = false;
int CCChannel = 1;
char MIDI_CONTROL_CHANGE_CH[] = {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF}; // send control change on channel 1-16, 
bool configCCmessage = false;
int CCmessage = 0;
char MIDI_CONTROL_NUMBER[] = {0x36,0x37}; // stutter time (volca beats), stutter depth (volca beats) // find another way to cfg this

char MIDI_NOTES[16]; // keep notes in memory along with interval at which to trigger the note off message
int MIDI_NOTES_DELAYED_OFF[16] = {0};
int CV_TRIGGER_OFF[16] = {0};
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
#define MIDI_NOTE_VEL_OFF 0x00 // 0 // note off
#define MIDI_SONG_POSITION_POINTER 0xF2

///// LEDC for PWM // CV Pitch /////
extern "C" {

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (23)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY              (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
// #define LEDC_LS_TIMER          LEDC_TIMER_1
// #define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE

ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_HS_MODE,             // Timer mode
    .duty_resolution = LEDC_TIMER_13_BIT,   // Resolution of PWM duty
    .timer_num = LEDC_HS_TIMER,             // Timer index
    .freq_hz = 5000,                        // Set output frequency at 5 kHz
    .clk_cfg = LEDC_AUTO_CLK                // Auto select the source clock
  };

ledc_channel_config_t ledc_channel = {
    .gpio_num   = LEDC_HS_CH0_GPIO,
    .speed_mode = LEDC_HS_MODE,
    .channel    = LEDC_HS_CH0_CHANNEL,
    .timer_sel  = LEDC_HS_TIMER,
    .duty       = 0,
    .hpoint     = 0     
  };

} // End Extern "C" 

/////End Ledc // CV_PITCH ///////////


///// seq /////
bool loadedSeq[1264] = {}; // to store the loaded sequences

int sensorValue = -42;
int previousSensorValue = 0;
int test = 777;
bool changeBPM;
bool changedMstr = false;

// Attempt at storing a sequence as an array of struct //
// Need to find out how to make it possible to store two notes on the same step. For example, there might be a 'Bass drum' and a High hat' on the same step.

typedef struct {
    bool on;          
    uint8_t chan;
    uint8_t bar;
    uint8_t note;
    uint8_t length;
    bool mute;
} steps_t;

steps_t steps[64]; // Declare an array of type struct steps

bool mstr[79] = {}; // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
bool oldmstr[79] = {1}; 
bool mtmss[1264] = {0}; // 79 x 16 géant et flat (save this for retrieval, add button to select and load them)

int channel; // 4 bits midi channel (0-15) -> // drums + más
int note; // 4 bits note info // 8 notes correspond to 8 colors // (0-7) -> (36,38,43,50,42,46,39,75),67,49 // más de 8 !
float duration[8] = {0.25,0.5,1,2,4,8,16,64}; // 4 bits note duration (0-7) -> (64,16,8,4,2,1,1/2,1/4)
float noteDuration; 
int myNoteDuration;
int bar[8] = {1,1,1,1,1,1,1,1}; // 2 bits, up to 4 bars?
int myBar = 0; 
bool muteRecords[8] = {0,0,0,0,0,0,0,0}; // mute info per note
int stepsLength[4] = {16,32,48,64}; // varies per note 16-64

int beat = 0; 
int step = 0 ;
int dubStep = 0;
float oldstep;
int currStep = 0;

static void periodic_timer_callback(void* arg);
esp_timer_handle_t periodic_timer;

bool startStopCB = false; // l'état du callback 
bool startStopState = false; // l'état local

bool isPlaying = true;

bool changePiton = false; 
bool changeLink = false;
bool tempoINC = false; // si le tempo doit être augmenté
bool tempoDEC = false; // si le tempo doit être réduit
double newBPM; // send to setTempo();
double oldBPM; 
double curr_beat_time;
double prev_beat_time;

bool connektMode = true; // flag pour envoyer l'adresse IP aux clients
char str_ip[16] = "192.168.0.66"; // send IP to clients !! // stand in ip necessary for memory space?
int nmbrClients = 0;
int loadedClients = 0;

int nmbrSeq = 0; // the number of sequences in nvs
char sequence[8][10] = {{"sequence"},{"seq1"},{"seq2"},{"seq3"},{"seq4"},{"seq5"},{"seq6"},{"seq7"}};


///////////// INTERACTIONS ///////////
///
/// save (Touch pad 2)
bool tapeArch = false; // flag for saving
bool saveBPM = false; 
bool saveSeq = false; // nvs save the sequence to start
bool seqSaved = false;
bool saveSeqConf = false;
bool loadSeq = false;
bool seqLoaded = false;
bool seqToLoad = false; // to know when to send the loaded sequence to clients
bool loadSeqConf = false;
bool saveDelay = false; // for when to remove the save options after an interaction
int delset = 0;
int selectedSeq = 0; // user selected sequence to load

///////////// TAP TEMPO //////////////
/// from : https://github.com/DieterVDW/arduino-midi-clock/blob/master/MIDI-Clock.ino

int foisTapped = 0;
bool toTapped = false;

int tapped = 0;
int lastTapTime = 0;
int tapInterval = 0;
int tapArray[5] = {0};
int tapTotal = 0;

//long intervalMicroSeconds;
int bpm;  // BPM in tenths of a BPM!!

int tappedBPM = 0;
int minimumTapInterval = 50;
int maximumTapInterval = 1500;

/* char numToASCII(int num) {
  return (char)num;
  } */


///////////// I2C Display ////////////
#if defined USE_I2C_DISPLAY
extern "C" {
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

char buf[20]; // BPM display
char compte[8];
char current_phase_step[20]; // 4

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

      touch_pad_read_filtered((touch_pad_t)0, &touch_value);
      s_pad_init_val[0] = touch_value;
      ESP_LOGI(TOUCH_TAG, "test init: touch pad [%d] val is %d", 0, touch_value); //set interrupt threshold.
      ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)0, touch_value * 2 / 3));

      touch_pad_read_filtered((touch_pad_t)2, &touch_value);
      s_pad_init_val[2] = touch_value;
      ESP_LOGI(TOUCH_TAG, "test init: touch pad [%d] val is %d", 2, touch_value); 
      ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)2, touch_value * 2 / 3));
  
      touch_pad_read_filtered((touch_pad_t)3, &touch_value);
      s_pad_init_val[3] = touch_value;
      ESP_LOGI(TOUCH_TAG, "test init: touch pad [%d] val is %d", 3, touch_value); 
      ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)3, touch_value * 2 / 3));

    for (int i = 5; i < 10; i=i+2) { 
      touch_pad_read_filtered((touch_pad_t)i, &touch_value);
      s_pad_init_val[i] = touch_value;
      ESP_LOGI(TOUCH_TAG, "test init: touch pad [%d] val is %d", i, touch_value);
      ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)i, touch_value * 2 / 3));
    }
}
static void tp_example_read_task(void *pvParameter) {
    
    
 while (1) {
     
    touch_pad_intr_enable();

        if (s_pad_activated[2] == true) { // 0
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 2);  // Wait a while for the pad being released // 0

        if(loadSeq == true ) { // if we had a previous touch then save that thang
          loadSeqConf = true;
        }

        else { // set it to true along with a delay
          loadSeq = true;
          delset = esp_timer_get_time()+3000000;
          ESP_LOGI(TAP_TAG, "loadSeq open until : %i", delset); 
          
          nmbrSeq = 0; // reset the number of sequences

          // Example of listing all the key-value pairs of any type under specified partition and namespace
          nvs_iterator_t it = nvs_entry_find("nvs", "mtmss", NVS_TYPE_ANY);
          // how many sequences do we have ?

          while (it != NULL) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            it = nvs_entry_next(it);
            printf("key '%s', type '%d' \n", info.key, info.type);
            nmbrSeq++; // add one to the sequences 
            ESP_LOGI(TAP_TAG, "nmbrSeq : %i", nmbrSeq); 
          };
          // Note: no need to release iterator obtained from nvs_entry_find function when
          //       nvs_entry_find or nvs_entry_next function return NULL, indicating no other
          //       element for specified criteria was found.

        }
      
        vTaskDelay(300 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[2] = false; // Reset the counter triggering a message // that application is running // 0
   
        } else if (s_pad_activated[3] == true) { // 2
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 3);  // Wait a while for the pad being released // 2
        
        if ( configCC == true && configCCChannel == true ){
          configCCChannel = false; // Current selected channel is frozen
          ESP_LOGI(TOUCH_TAG, "Channel is selected");
        }
        else if ( configCC == true && configCCmessage == true ){
          configCCmessage = false; // No more changes to the CC messages
          configCC = false; 
          need2configCC = false; 
        }
        
        else {

        if ( saveBPM == true ) {
          tapeArch = true; // flag for saving 
        }

        else if ( saveSeq == true ) {
          saveSeqConf = true;
        }

        else if ( saveBPM == false ) {
          saveSeq = true;
          delset = esp_timer_get_time()+3000000;
          ESP_LOGI(TAP_TAG, "saveSeq open until : %i", delset); 
        }

        }

     
        vTaskDelay(300 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[3] = false; // Reset the counter triggering a message // that application is running // 2
        
        } else if (s_pad_activated[5] == true) { // 3
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 5);  // Wait a while for the pad being released // 3
        //ESP_LOGI(TOUCH_TAG, "TAP!");
        toTapped = true;
        vTaskDelay(300 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[5] = false; // Reset the counter triggering a message // that application is running // 3

        } else if (s_pad_activated[7] == true) { // 5
        ESP_LOGI(TOUCH_TAG, "T%d piton!", 7);  // 5
        startStopState = !startStopState; 
        changePiton = true;
        ESP_LOGI(TOUCH_TAG, "startStopState : %i ", startStopState);
        ESP_LOGI(TOUCH_TAG, "changePiton : %i ", changePiton);
        vTaskDelay(300 / portTICK_PERIOD_MS);  
        s_pad_activated[7] = false;  // 5
        
        } else if (s_pad_activated[0] == true) { // 7
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 0);  // 7

          if (loadSeq == true){
            selectedSeq--;
            delset = esp_timer_get_time()+3000000;
            if (selectedSeq < 0 ){
              selectedSeq = nmbrSeq-1; // loop it
            }
          }
          
          else{
            tempoDEC = true; 
          }
        
        vTaskDelay(300 / portTICK_PERIOD_MS);  
        s_pad_activated[0] = false; // 7


        } else if (s_pad_activated[9] == true) {
        ESP_LOGI(TOUCH_TAG, "T%d activated!", 9);  // Wait a while for the pad being released

          
          if (loadSeq == true){
            selectedSeq++;
            delset = esp_timer_get_time()+3000000;

            if (selectedSeq > nmbrSeq ){
            selectedSeq = 0; // loop it
            }
          }
          
          else{
            tempoINC= true;  // pour que le audio loop le prenne en compte
          }     

        vTaskDelay(300 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[9] = false; // Reset the counter triggering a message // that application is running
        }
        
    vTaskDelay(10 / portTICK_PERIOD_MS);
     
    }
}


static void tp_example_rtc_intr(void *arg) { //  Handle an interrupt triggered when a pad is touched. Recognize what pad has been touched and save it in a table.
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status(); //clear interrupt

     
        if ((pad_intr >> 0) & 0x01) {
            s_pad_activated[0] = true;
        }
        else if  ((pad_intr >> 2) & 0x01){
            s_pad_activated[2] = true;
        }
        else if  ((pad_intr >> 3) & 0x01){
            s_pad_activated[3] = true;
        }

    for (int i = 5; i < 10; i = i+2) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}


static void tp_example_touch_pad_init(void) { // Before reading touch pad, we need to initialize the RTC IO.
  
    touch_pad_config((touch_pad_t)0, TOUCH_THRESH_NO_USE); 
    touch_pad_config((touch_pad_t)2, TOUCH_THRESH_NO_USE); 
    touch_pad_config((touch_pad_t)3, TOUCH_THRESH_NO_USE); 
    for (int i = 5; i < 10; i = i+2) {   
        touch_pad_config((touch_pad_t)i, TOUCH_THRESH_NO_USE); //init RTC IO and mode for touch pad.
    }
}

}    
#endif

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


////////////////////// sockette server ///////////////////////
#if defined USE_SOCKETS

int sock; 

extern "C" {
  static void udp_server_task(void *pvParameters)
  {

    // Du client Sockette _task
    // Trying to send and OSC message over UDP from the server...
    /* 
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR_WT32); 
    dest_addr_WT32.sin_family = AF_INET;
	  dest_addr_WT32.sin_port = htons(PORT); // "3334"	
	  addr_family = AF_INET;
	  ip_protocol = IPPROTO_IP;
  	inet_ntoa_r(dest_addr_WT32.sin_addr, addr_str, sizeof(addr_str) - 1); // Dunno about the addr_str
       
    // Trying to send a broadcast message :/
    int enabled = 1;

    setsockopt(sock_WT32, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));

 	  sock_WT32 = socket(addr_family, SOCK_DGRAM, ip_protocol);

  	if (sock_WT32 < 0) {
    	ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
  		}

  	ESP_LOGI(SOCKET_TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR_WT32, PORT_WT32);
    ESP_LOGE(SOCKET_TAG, "sock vaut : %i", sock_WT32);

    char monBuffer[16]; // monBuffer[16] // // declare a buffer for writing the OSC packet into
    uint8_t stepper = 42;                                     

    int maLen = tosc_writeMessage(
        monBuffer, sizeof(monBuffer),
        "/tdlf", // the address
        "i",   // the format; 'f':32-bit float, 's':ascii string, 'i':32-bit integer
        stepper);

        int err2 = sendto(sock_WT32, monBuffer, maLen, 0,(struct sockaddr *)&dest_addr_WT32, sizeof(dest_addr_WT32));
   
        if (err2 < 0) {
          ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            //break;
          } 
    
          ESP_LOGI(SOCKET_TAG, "Message sent WT_32");

            

            if (err < 0) {
              ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
              break;
              }  

    // Fin client sockette task 
    */

    char mstr[16]; // char mstr[64]
    char addr_str[128];
    int addr_family = AF_INET6;
    int ip_protocol = IPPROTO_IPV6;
    struct sockaddr_in6 dest_addr; // IPV6

    while (1) { 

        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);

        sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

        ESP_LOGI(SOCKET_TAG, "Socket created id : %i", sock);

        if (sock < 0) {
            ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket created");

        int err1 = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err1 < 0) {
            ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket bound, port %d", PORT);

        while (1) {

          ESP_LOGI(SOCKET_TAG, "Waiting for data\n");

          struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
          socklen_t socklen = sizeof(source_addr);
            
          //mstr should be 79 values long;
          int len = recvfrom(sock, mstr, sizeof(mstr), 0, (struct sockaddr *)&source_addr, &socklen);
           
          for(int i = 0; i<sizeof(mstr); i++){
            test = mstr[i];
            ESP_LOGE(SOCKET_TAG, "data %i", test); 
          }  

          /////////////////// FROM TINYOSC ///////////////////////
          //tosc_printOscBuffer(rx_buffer, len);
          tosc_printOscBuffer(mstr, len);

            tosc_message osc; // declare the TinyOSC structure
            char buffer[1024]; // char buffer[64]; declare a buffer into which to read the socket contents
            //int oscLen = 0; // the number of bytes read from the socket

            // tosc_parseMessage(&osc, rx_buffer, len);
            tosc_parseMessage(&osc, mstr, len);

            //tosc_printMessage(&osc);
            tosc_getAddress(&osc),  // the OSC address string, e.g. "/button1"
            tosc_getFormat(&osc);  // the OSC format string, e.g. "f"

            //printf("%s %s", 
            ////&osc->len,              // the number of bytes in the OSC message
            //tosc_getAddress(&osc),  // the OSC address string, e.g. "/button1"
            //tosc_getFormat(&osc));  // the OSC format string, e.g. "f"

           //switch (tosc_getFormat(&osc)) {
            switch(osc.format[0]) {
    
                case 'm': {
                    unsigned char *m = tosc_getNextMidi(&osc);
                    printf(" 0x%02X%02X%02X%02X", m[0], m[1], m[2], m[3]);
                    printf("\n");
                    //CCChannel = m[0];
                    //sensorValue = m[1];
                    //CCmessage = m[2];
                    steps[m[3]].on = m[2]; // m[2] is on/off info and m[3] is the step number

                    // If the sequencer is going to only be 16 steps...we might not need all these 64...
                    /* for (int i = 0 ; i < 64 ; i++ ){
                      ESP_LOGI(SEQ_TAG, "step : %i, note value :%i", i, steps[i].on); 
                    } */

                    //printf("%02X",m[0]);
                    //printf("\n");
      
                    break;
                }
    
                default:
                    printf(" Unknown format: '%c'", osc.format[0]);
                    break;
                } 
  
                printf("\n");
            /////////////////// END TINYOSC ///////////////////////

            ///// midi channel ///// 1-16 midi channels
            // ESP_LOGI(SOCKET_TAG, "channel : %i", channel); 
  
            ///// note /////
            ESP_LOGI(SOCKET_TAG, "note : %i", note); 

            ///// noteDuration //////
            // ESP_LOGI(SOCKET_TAG, "noteDuration : %f", noteDuration); 

            ///// Step ///// 0-63 steps
            //ESP_LOGI(SOCKET_TAG, "note : %i", note); // Change this
            // Where is the note going in the sequence
          
            // read in the bit value for mute and store 
            muteRecords[note] = mstr[14];
            // ESP_LOGI(SOCKET_TAG, "mute ? : %i", mstr[10]);  
          
            // Error occurred during receiving ?
            if (len < 0) {
                ESP_LOGE(SOCKET_TAG, "recvfrom failed: errno %d", errno);
                break;
              }
            
            else {   // Data received
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1); // Get the sender's ip address as string
              }
                
           ESP_LOGI(SOCKET_TAG, "Received %d bytes from %s:", len, addr_str);
                
            //inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1); // Get the sender's ip address as string
                
            if ( startStopState == false ) { // only check for clients if we are in stopped mode, it hangs the playback otherwise...
              
              int checkIPExist = clientIPCheck(addr_str); // Does it exist in the array?

             // ESP_LOGI(SOCKET_TAG, "result of clientIPCheck : %i", checkIPExist);

              if ( checkIPExist == 42 ) { // if it doesn't exist, add it
              
                strncat(clientIPAddresses[nmbrClients], inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1),20); // add that address to the array 
                ESP_LOGI(SOCKET_TAG, "Added client address : %s", clientIPAddresses[nmbrClients]);

                nmbrClients++; // Count the newly registered client
                ESP_LOGI(SOCKET_TAG, "How many clients ? : %i", nmbrClients);

                }

              else { // IP already exists 

                ESP_LOGI(SOCKET_TAG, "Address already exists : %s", addr_str);    // ip address already exists in the array so do nothing // at what position
                
                if ( seqToLoad ) { // 

                  ESP_LOGI(SOCKET_TAG, "Sending love instead of an IP to : %s", addr_str); 

                  bool tmpArray[79];
                  
                  for ( int i = 0; i < 80; i++ ) { // checkIPExist is the offset

                    tmpArray[i] = loadedSeq[i+(79*checkIPExist)]; // populate tmpmstr array
                    ESP_LOGI(SOCKET_TAG, "tmpArray[%i] = %i ", i, tmpArray[i]); 

                  }

                  int err = sendto(sock, tmpArray, sizeof(tmpArray), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                  loadedClients++; // keep track of how many clients have been loaded
                  ESP_LOGI(SOCKET_TAG, "loadedClients : %i\n", loadedClients); 
                  
                  if( loadedClients == nmbrClients ) { // All the clients are loaded

                    seqToLoad = false;
                    loadedClients = 0;

                    }
                    
                }

              } // End of "else if IP exists"

              ESP_LOGI(SOCKET_TAG, "nmbrClients : %i\n", nmbrClients); 
            
            } 

            int err = sendto(sock, str_ip, sizeof(str_ip), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            ESP_LOGI(SOCKET_TAG, "Sent my IP %s", str_ip); 
            
            
     
        } // End of while

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
      bzero(&wifi_config, sizeof(wifi_config_t)); // ... or wifi_config_t wifi_config = { }; // when declaring wifi_config_t structure, do not forget to set all fields to zero.
      
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

  #if defined USE_I2C_DISPLAY   
        if ( DefaultBusInit( ) == true ) {

        printf( "BUS Init lookin good...\n" );
       
        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Technologies", SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "de la fete", SSD_COLOR_WHITE );
        SSD1306_SetVFlip( &I2CDisplay, 1 ); 
        SSD1306_SetHFlip( &I2CDisplay, 1 ); //void SSD1306_SetHFlip( struct SSD1306_Device* DeviceHandle, bool On );
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
   
    ESP_LOGI(NVS_TAG,"WIFI_STA ssid :%s",lssid); 
    ESP_LOGI(NVS_TAG,"WIFI_STA password :%s",lpassword); 

    wifi_config_t wifi_config;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

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

        // ESP_LOGI(WIFI_TAG, "Connected to WiFI");
        
        #if defined USE_I2C_DISPLAY   
          SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
          SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
          SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, str_ip, SSD_COLOR_WHITE ); 
          SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24 );
          SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "Link!", SSD_COLOR_WHITE );
          SSD1306_Update( &I2CDisplay );  
        #endif 
        
    } else if (bits & WIFI_FAIL_BIT) {

        ESP_LOGE(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
        wifi_config.sta.ssid, wifi_config.sta.password);
        goSMART = true; // pour la suite des choses

        #if defined USE_I2C_DISPLAY 
          SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
          SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13 );
          SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "Use", SSD_COLOR_WHITE );
          SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, "ESPTouch", SSD_COLOR_WHITE );
          SSD1306_Update( &I2CDisplay ); 
        #endif
        
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }
    /* The event will not be processed after unregister */
   // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
   // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //vEventGroupDelete(s_wifi_event_group);
  } 
} // fin extern "C"


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
    ESP_LOGI(LINK_TAG, "tempochanged");

// Du client Sockette _task
    dest_addr_WT32.sin_addr.s_addr = inet_addr(HOST_IP_ADDR_WT32); //  "192.168.50.240"
    dest_addr_WT32.sin_family = AF_INET;
	  dest_addr_WT32.sin_port = htons(PORT_WT32); // "3334"	
	  addr_family = AF_INET;
	  ip_protocol = IPPROTO_IP;
  	inet_ntoa_r(dest_addr_WT32.sin_addr, addr_str, sizeof(addr_str) - 1); // Dunno about the addr_str

    // Trying to send a broadcast message :/ 1 yes 0 no....
    //int enabled = 0;
    //setsockopt(sock_WT32, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    int flag = 1;
    setsockopt(sock_WT32, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

 	  sock_WT32 = socket(addr_family, SOCK_DGRAM, ip_protocol);

  	if (sock_WT32 < 0) {
    	ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
  		}

  	ESP_LOGI(SOCKET_TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR_WT32, PORT_WT32);
    ESP_LOGE(SOCKET_TAG, "sock vaut : %i", sock_WT32);

        int err3 = bind(sock_WT32, (struct sockaddr *)&dest_addr_WT32, sizeof(dest_addr_WT32));

        if (err3 < 0) {
            ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            //break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket bound, port %d", PORT_WT32);


    char monBuffer[16]; // monBuffer[16] // // declare a buffer for writing the OSC packet into
    uint8_t stepper = 43; 

    //ici!
    
    steps[0].on = true; // Test writing to the array of structs
    ESP_LOGI(SEQ_TAG, "First step : %i", steps[0].on);                                   

    int maLen = tosc_writeMessage(
        monBuffer, sizeof(monBuffer),
        "/tdlf", // the address // /a second level like /tdlf/step returns an error '122'
        "i",   // the format; 'f':32-bit float, 's':ascii string, 'i':32-bit integer
        stepper);

        int err2 = sendto(sock_WT32, monBuffer, maLen, 0,(struct sockaddr *)&dest_addr_WT32, sizeof(dest_addr_WT32));
   
        if (err2 < 0) {
          ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            //break;
          } 
    
          ESP_LOGI(SOCKET_TAG, "Message sent WT_32");

if (sock_WT32 != -1) {
        ESP_LOGE(SOCKET_TAG, "Shutting down socket and restarting...");
        shutdown(sock_WT32, 0);
        close(sock_WT32);
    }

    // Fin client sockette task


    double midiClockMicroSecond = ((60000 / tempo) / 24) * 1000;

    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) periodic_timer;
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, midiClockMicroSecond));
}

void startStopChanged(bool state) {
  // received as soon as sent, we can get the state of 'isPlaying' and use that
  // need to wait for phase to be 0 (and deal with latency...)
  startStopCB = state;
  changeLink = true;

  //ESP_LOGI(LINK_TAG, "startstopCB is : %i", state);
  //ESP_LOGI(LINK_TAG, "changeLink is : %i", state);
}



void delayer(int del) {

  delset = esp_timer_get_time()+del;
  ESP_LOGI(TAP_TAG, "delaying until : %i", delset); 

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
 
    xSemaphoreTake((QueueHandle_t)userParam, portMAX_DELAY);

    const auto state = link.captureAudioSessionState(); 
    isPlaying = state.isPlaying();
    //  ESP_LOGI(LINK_TAG, "isPlaying : , %i", isPlaying);  
    //const auto phase = state.phaseAtTime(link.clock().micros(), 1); 
    //ESP_LOGI(LINK_TAG, "tempoINC : %i", tempoINC);
    //ESP_LOGI(LINK_TAG, "tempoDEC : %i", tempoDEC);
    //ESP_LOGI(LINK_TAG, "saveSeqConf : %i", saveSeqConf);
    //ESP_LOGI(LINK_TAG, "toTapped : %i", toTapped );

    if ( saveSeqConf == true ) {

      /////// WRITING mtmss[] TO NVS //////
      nvs_handle wificfg_nvs_handler;
      nvs_open("mtmss", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_set_blob(wificfg_nvs_handler,sequence[0],mtmss,1264);
      nvs_commit(wificfg_nvs_handler); 
      nvs_close(wificfg_nvs_handler); 
      ////// END NVS ///// 
      
      seqSaved = true;
      delset = esp_timer_get_time()+1500000;
      saveSeqConf = false;
      
      for (int i = 0; i < 180; i++) { // What are we writing?
       ESP_LOGI(SOCKET_TAG, "savedSeq[%i] = %i ", i, mtmss[i]);   
      } 
      ESP_LOGI(NVS_TAG, "Sequence saved");
  
      
    }

    if ( loadSeqConf == true ) {

      /// NVS READ CREDENTIALS ///

       loadSeqConf = false;

      nvs_handle wificfg_nvs_handler;
      size_t len;
      nvs_open("mtmss", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_get_blob(wificfg_nvs_handler, sequence[selectedSeq], NULL, &len);
      char* mySeq = (char*)malloc(len); 
      nvs_get_blob(wificfg_nvs_handler, sequence[selectedSeq], mySeq, &len);
      nvs_close(wificfg_nvs_handler);

      ///// END NVS /////

     for (int i = 0; i < 242; i++) { // Populate the tmp loadedSequence from nvs
       loadedSeq[i] = mySeq[i];
       ESP_LOGI(SOCKET_TAG, "loadedSeq[%i] = %i ", i, loadedSeq[i]);   
      } 
  
      seqLoaded = true;
      seqToLoad = true;
      delset = esp_timer_get_time()+2000000;
      ESP_LOGI(NVS_TAG, "Sequence #%i loaded", selectedSeq);

    }

    if ( changeBPM == true ) {
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      
      link.commitAppSessionState(mySession); 
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
    }

    if ( tempoINC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo + 1;
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession); // le problème est que l'instruction de changer le tempo nous revient
      tempoINC = false;
      saveBPM = false; // as changing the BPM here implies not wanting to tap the tempo in
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
      //ESP_LOGI(LINK_TAG, "tempoINC : %i", tempoINC);
    }

    if ( tempoDEC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo - 1;
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession);
      tempoDEC = false;
      saveBPM = false;
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
    }

    ////////// test start stop send to other clients /////////
    //ESP_LOGI(LINK_TAG, "PRE startStopState is :  %i", startStopState);  
    //ESP_LOGI(LINK_TAG, "PRE startStopCB is :  %i", startStopCB); 

    if ( changePiton && startStopState != startStopCB ){ // if local state is different to the SessionState then send to the session state 
        auto mySession = link.captureAppSessionState();
        const auto timez = link.clock().micros();
        mySession.setIsPlaying(startStopState, timez);
        link.commitAppSessionState(mySession);
      }
      
    if ( changeLink && startStopState != startStopCB ){ // if CB state is different to the local startStopState then resync the latter to the former
       startStopState = startStopCB; // resync 
      }

    if( toTapped == true ){

      tapped = esp_timer_get_time()/1000;
      tapInterval = tapped - lastTapTime;

      // ESP_LOGI(TAP_TAG, "Time at tap: %ld ", tapped);
      ESP_LOGI(TAP_TAG, "Time at tap: %i ", tapped);
      ESP_LOGI(TAP_TAG, "Interval between taps n: %i ", tapInterval );

      if(tapInterval > maximumTapInterval){ // reset tapCounter to zero
        foisTapped = 0;
      }

      if(minimumTapInterval < tapInterval && tapInterval < maximumTapInterval){ // only add a tap if we are within the tempo bounds
        
        foisTapped = foisTapped + 1;
   
        ESP_LOGI(TAP_TAG, "foisTapped %i", foisTapped);

        // add a tapInterval to the sliding values array

        tapArray[foisTapped%5] = tapInterval;

      }
      
      if(foisTapped > 5){  // calculate BPM from tapInterval

        for(int i = 0; i<6; i++){
          tapTotal = tapArray[i] + tapTotal;
          ESP_LOGI(TAP_TAG, "tapped interval at %i, %i", i, tapArray[i]);

        }
        tapTotal = tapTotal / 6;

        tappedBPM = 60L * 1000 / tapTotal;

        ESP_LOGI(TAP_TAG, "tapped BPM %i", tappedBPM);

        tapTotal = 0;
        saveBPM = true;
        saveDelay = true; // appeler une fonction pour remettre les variables à false après un moment
        delayer(3000000);
      }

      lastTapTime = tapped;

      toTapped = false;

    }

    if (esp_timer_get_time() > delset ){
     saveBPM = false;

     saveSeq = false; 
     seqSaved = false;
     
     loadSeq = false;
     seqLoaded = false;
   }

    if ( saveBPM == true && tapeArch == true ) {
      
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(tappedBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession);
      saveBPM = false;
      tapeArch = false;

    }

    //ESP_LOGI(LINK_TAG, "POST startStopState is :  %i", startStopState);  
    //ESP_LOGI(LINK_TAG, "POST startStopCB is :  %i", startStopCB); 
   
    curr_beat_time = state.beatAtTime(link.clock().micros(), 4);
    const double curr_phase = fmod(curr_beat_time, 4);  // const double

    if ( curr_beat_time > prev_beat_time ) {

      // try sending cc messages here 
      if (sensorValue > -42 && configCC == false){

        char zedata1[] = { MIDI_CONTROL_CHANGE_CH[CCChannel-1] }; // send CC message on midi channel 1 '0xB0'
        uart_write_bytes(UART_NUM_1, zedata1, 1); // this function will return after copying all the data to tx ring buffer, UART ISR will then move data from the ring buffer to TX FIFO gradually.
      
        char zedata2[] = { (char)CCmessage};  //  '0x36' (54) trying for Volca Beats Stutter time
        uart_write_bytes(UART_NUM_1, zedata2, 1); 
      
        char zedata3[] = { (char)sensorValue }; // need to convert sensorValue to hexadecimal! 
        uart_write_bytes(UART_NUM_1, zedata3, 1); 

        if (sensorValue != previousSensorValue){
        // ESP_LOGI(CV_TAG, "CV_PITCH, %i", int(sensorValue));
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, int(sensorValue)); // CV_PITCH
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        previousSensorValue = sensorValue;
        }
        
      }
      
      const double prev_phase = fmod(prev_beat_time, 4);
      const double prev_step = floor(prev_phase * 4);
      const double curr_step = floor(curr_phase * 4);
      
      //ESP_LOGI(LINK_TAG, "current step : %f", curr_step); 
      //ESP_LOGI(LINK_TAG, "prev step : %f", prev_step); 
     
      if (abs(prev_phase - curr_phase) > 4 / 2 || prev_step != curr_step) { // quantum divisé par 2

        if( ( curr_step == 0 && changePiton ) || ( curr_step == 0 && changeLink ) ) { // start / stop implementation
            
              if(startStopCB) { // saw a simpler way of doing this!
                char zedata[] = { MIDI_START };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                //uart_write_bytes(UART_NUM_1, 0, 1); // ? produces : E (24513) uart: uart_write_bytes(1112): buffer null
                changeLink = false;
                changePiton = false;
                step = 0; // reset step count
              } 
              else { // on jouait et on arrête // changer pout s'arrêter immédiatement après un stop 
                char zedata[] = { MIDI_STOP };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                //uart_write_bytes(UART_NUM_1, 0, 1);
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
        // ESP_LOGI(LINK_TAG, "tempo %i", tmpOH); 

        char phases[5] = "TDLF";

        char tmpOHbuf[20];
        char top[20];
        char ipa[16];

        if ( seqSaved == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Saved   ");
        }

        else if ( saveSeq == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Save?   ");
        }

        else if ( seqLoaded == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "%i loaded", selectedSeq);
        }

        else if ( loadSeq == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Load:+%i-?", selectedSeq);
        }
        
        else if (saveBPM == false){
          snprintf(tmpOHbuf, 20 , "%i BPM", tmpOH );     /////// display BPM + Phase + Step /////////

          if (step < 10){
            snprintf(current_phase_step, 20, " 0%i STP", step);
          }
          else {
            snprintf(current_phase_step, 20, " %i STP", step);
          }

        }
        
        else if (saveBPM == true){ // we have a tapped BPM ready to switch?
          snprintf(tmpOHbuf, 20 , "%i BPM?", tappedBPM ); 
          
          if (step < 10){
            snprintf(current_phase_step, 20, " 0%i STP", step);
          }
          else {
            snprintf(current_phase_step, 20, " %i STP", step);
          }

        }

    

      //ESP_LOGI(LINK_TAG, "%i", halo_welt); 
      switch (halo_welt)
      {
        case 0:
        strcpy(phases, "X000");
        break;
        case 1:
        strcpy(phases, "XX00");
        break;
        case 2:
        strcpy(phases, "XXX0"); 
        break;
        case 3:
        strcpy(phases, "XXXX");
        break;
        default:
        ESP_LOGI(LINK_TAG, "phases not assigned correctly"); 
      }


        snprintf(top, 20, "clients:%i  %s", nmbrClients, phases);
        SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );

        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13);
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, top, SSD_COLOR_WHITE ); 

        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24); // &Font_droid_sans_mono_13x24 // &Font_droid_sans_fallback_15x17
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, tmpOHbuf, SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_South, current_phase_step, SSD_COLOR_WHITE );
        SSD1306_Update( &I2CDisplay );  
      

        if (startStopCB){ // isPlaying and did we send that note out? 

          // ESP_LOGI(LINK_TAG, "step : %i", step);
          // ESP_LOGI(LINK_TAG, "sizeof mtmss : %i", sizeof(mtmss));

              //// shut off the midi notes that need to be ////
    int monTemps = int(esp_timer_get_time()/1000);
    //ESP_LOGI(MIDI_TAG, "Mon Temps, %i", monTemps);
      //for( int i = 0; i < 8; i++ ) {
        //if( MIDI_NOTES_DELAYED_OFF[i] > 0 && MIDI_NOTES_DELAYED_OFF[i] < monTemps ) {

      if( CV_TRIGGER_OFF[0] > 0 && CV_TRIGGER_OFF[0] < monTemps ) {
          gpio_set_level(CV_18, 0); // DAC_D // CV Trigger
          // gpio_set_level(CV_5, 0); // DAC_B // CV Clock Out
          // gpio_set_level(CV_23, 0); // DAC_C // CV Pitch
        }

      if( MIDI_NOTES_DELAYED_OFF[0] > 0 && MIDI_NOTES_DELAYED_OFF[0] < monTemps ) {
        // ESP_LOGI(MIDI_TAG, "Should attempt to turn this off : %i", i);
        // ESP_LOGI(MIDI_TAG, "Should attempt to turn this off ");

          //char zedata0[] = {MIDI_NOTE_OFF};
          //uart_write_bytes(UART_NUM_1,zedata0,1); // note off before anything
          
        char zedata1[] = { MIDI_NOTE_ON_CH[1] }; // défini comme midi channel 1 (testing)
        uart_write_bytes(UART_NUM_1, zedata1, 1); //uart_write_bytes(UART_NUM_1, "0x90", 1); // note on channel 0
          char zedata2[] = {MIDI_NOTES[0]}; // tableau de valeurs de notes hexadécimales 
          //char zedata2[] = {MIDI_NOTES[i]}; // tableau de valeurs de notes hexadécimales 
          uart_write_bytes(UART_NUM_1, zedata2, 1); 
          char zedata3[] = {MIDI_NOTE_VEL_OFF};
          uart_write_bytes(UART_NUM_1,zedata3, 1); // velocité à 0
        }
    //} // end midi note off

          for(int i = 0; i<8;i++){ // 8 x 79 = 632 faut faire ça pour chaque valeur de note
           
              myBar =  mtmss[i*79+12] + (mtmss[i*79+13])*2; // 0, 1, 2, 3 bars // how many bars for this note?
              
              //ESP_LOGI(LINK_TAG, "myBar : %i", myBar);
              //ESP_LOGI(LINK_TAG, "stepsLength[myBar] : %i", stepsLength[myBar]);
              dubStep = step%stepsLength[myBar]; // modulo 16 // 32 // 48 // 64
              
              //ESP_LOGI(LINK_TAG, "nouveau 'dub'Step : %i", dubStep);

              currStep = (i*79)+dubStep+15; // 15 is where the step info starts
              // ESP_LOGI(LINK_TAG, "currStep : %i", currStep);

               /* for(int j = 0; j<79;j++){
                    ESP_LOGI(LINK_TAG, "mtmss : %i, %i", j, mtmss[j]);
               }  */ 

              if (mtmss[currStep] == 1){ // send midi note out // mute to be implemented // && !muteRecords[i]){ 
              // ESP_LOGI(LINK_TAG, "MIDI_NOTE_ON_CH, %i", channel);
              // ESP_LOGI(LINK_TAG, "Note on, %i", i);

                if (channel == 10){ // are we playing drumz // solenoids ?
                  char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme channel 10(drums), ou channel 1(synth base) pour l'instant mais dois pouvoir changer
                  uart_write_bytes(UART_NUM_1, zedata1, 1); // this function will return after copying all the data to tx ring buffer, UART ISR will then move data from the ring buffer to TX FIFO gradually.
                  char zedata2[] = {zeDrums[i]};      // arriver de 0-8
                  uart_write_bytes(UART_NUM_1, zedata2, 1); // tableau de valeurs de notes hexadécimales 
                  char zedata3[] = { MIDI_NOTE_VEL };
                  uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                  
                  gpio_set_level(CV_18,1); // DAC_D (CV_18) get that note out !    
                  CV_TRIGGER_OFF[0] = int((esp_timer_get_time()/1000)+(myNoteDuration/64)); // set trigger duration
                }

              else if (channel == 5){ // synth, {0x99,0x90};

                char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme midi channel channel 0 
                // ESP_LOGI(MIDI_TAG, "MIDI_NOTE_ON_CH, %i", MIDI_NOTE_ON_CH[channel]);
                uart_write_bytes(UART_NUM_1, zedata1, 1); 

                char zedata2[] = {zeDark[i]}; // tableau de valeurs de notes hexadécimales 
                uart_write_bytes(UART_NUM_1, zedata2, 1); 

                char zedata3[] = { MIDI_NOTE_VEL };
                uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                  
                //const auto tempo = state.tempo(); // quelle est la valeur de tempo?
                //myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration

                // check if there is a value in the array and populate it or just add the note
                //int posArray = step%8;
                //ESP_LOGI(MIDI_TAG, "posArray, %i", posArray);

                //MIDI_NOTES[posArray] = zeDark[i];
                //MIDI_NOTES_DELAYED_OFF[posArray] = int((esp_timer_get_time()/1000)+myNoteDuration); // duration

                MIDI_NOTES[0] = zeDark[i];
                MIDI_NOTES_DELAYED_OFF[0] = int((esp_timer_get_time()/1000)+myNoteDuration); // duration

                ESP_LOGI(MIDI_TAG, "MIDI NOTE, %i", MIDI_NOTES[0]);
                ESP_LOGI(MIDI_TAG, " "); // new line
                // ESP_LOGI(MIDI_TAG, "MIDI TIME, %i", MIDI_NOTES_DELAYED_OFF[0]);

                }

                else{
                  ESP_LOGI(MIDI_TAG, "Midi channel other than 0 or 1"); // new line
                  // ajouter d'autres gammes (scales)
                } 
                
                //char zedata3[] = { MIDI_NOTE_VEL };
                //uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                uart_write_bytes(UART_NUM_1, "0", 1); // ??

              }
                  
          }
           // ESP_LOGI(LINK_TAG, "");
        }

        if(isPlaying){
          // ESP_LOGI(LINK_TAG, "step : %d", step); 
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
    // ESP_LOGI(LINK_TAG, "MIDI_TIMING_CLOCK");
    uart_write_bytes(UART_NUM_1, zedata, 1);

    // 24 times per second
    // faire modulo 6 pour arriver à 4 ticks par beat
    
    if(CV_TIMING_CLOCK % 6 == 0 ){
      gpio_set_level(CV_5, 1); // CV Clock Out
      // ESP_LOGI(CV_TAG, "CLOCK 1");
      CV_TIMING_CLOCK = 0;
    } else {
      gpio_set_level(CV_5, 0); 
      // ESP_LOGI(CV_TAG, "CLOCK 0");
    }  
    CV_TIMING_CLOCK = CV_TIMING_CLOCK+1;
    // ESP_LOGI(CV_TAG, "CV_TIMING_CLOCK, %i", CV_TIMING_CLOCK);

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
    //nvs_handle wificfg_nvs_handler;
    //size_t len;
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

  // CV out 
  //zero-initialize the config structure.
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  //disable pull-down mode
  //io_conf.pull_down_en = 0; // got an error here 
  //disable pull-up mode
  //io_conf.pull_up_en = 0; // got an error here
  //configure GPIO with the given settings
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
  
  //} // fin de goLINK

  //// LEDC //// PWM //// CV Pitch ////
  
  /* ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_HS_MODE,             // Timer mode
    .duty_resolution = LEDC_TIMER_13_BIT,   // Resolution of PWM duty
    .timer_num = LEDC_HS_TIMER,             // Timer index
    .freq_hz = 5000,                        // Set output frequency at 5 kHz
    .clk_cfg = LEDC_AUTO_CLK                // Auto select the source clock
  }; */

  ledc_timer_config(&ledc_timer);

  ledc_channel_config(&ledc_channel);

  ledc_fade_func_install(0);
  
  ///// End LEDC /////


  } // fin de extern "C"
} // fin de main()
