//// 26/10/2022
//// tdlf client // 16 touch pads // 16 leds // envoi des données de un array au client

// LEDS driver from https://github.com/limitz/esp-apa102
// Based on the ESP32 Ableton Link port by Mathias Bredholt https://github.com/mathiasbredholt/link-esp

//#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
//#include "protocol_examples_common.h"
#include <ableton/Link.hpp>
#include <driver/timer.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include <chrono> // essai pour setTempo()

///////// TOUCH //////
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

///////// SOCKETTE ////////
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define HOST_IP_ADDR "255.255.255.255" // trying to broadcast first //
#define PORT 3333

extern "C" {
static const char *SOCKET_TAG = "Socket";
static const char *SMART_TAG = "Smart config";
static const char *NVS_TAG = "NVS";
static const char *WIFI_TAG = "Wifi";
static const char *TAG = "tdlf";
}

////// sockette /////
char rx_buffer[79]; // To hold incoming sequence
char addr_str[128];
int addr_family;    
int ip_protocol;
struct sockaddr_in dest_addr;
int sock;
bool mstrpckIP = false;
bool nouvSockette = false;

bool bdChanged = false; // Was the sequence changed? Replace this with : 
bool sendData = false;  // Was the sequence changed? If so send the data over to the server

int press;
int selektor; // couleur rose par défaut
int modSelektor = 0;
int midiChannel = 0; // channel à stocker ds les premiers 0-3 bits de bd[]

// Starting note
// int noteSelektor = 0; // note à stocker dans les bits 4-7 de bd[] // WHITE-ISH // Done! // 36
// int noteSelektor = 1; // note à stocker dans les bits 4-7 de bd[] // RED // Done! // 38
// int noteSelektor = 2; // note à stocker dans les bits 4-7 de bd[] // AMBER // Done! // 43
// int noteSelektor = 3; // note à stocker dans les bits 4-7 de bd[] // YELLOW // Done! // 50
// int noteSelektor = 4; // note à stocker dans les bits 4-7 de bd[] // GREEN // Done! // 42
// int noteSelektor = 5; // note à stocker dans les bits 4-7 de bd[] // Done! // 46
// int noteSelektor = 6; // note à stocker dans les bits 4-7 de bd[] // PURPLE // 39
int noteSelektor = 7; // note à stocker dans les bits 4-7 de bd[] // LIGHT PURPLE // 75

int noteDuration = 0; // length of note stored in bits 8-11 of bd[] 
int barSelektor = 0; // à stocker
int currentBar = 0; // pour l'affichage

bool cleanSlate = true; // Need this to run once to erase the correpsonding sequence on the server.

/// OSC ///

uint8_t midi[4]; // This will hold the data to be sent over

///////// DELS // SPI CONFIG TTGO // VSPI // SPI3 // MOSI 23 // SCK 18
extern "C"{
#include "ESP32APA102Driver.h"

unsigned char colourList[9*3]={maxValuePerColour,0,0, maxValuePerColour,maxValuePerColour,0, 0,maxValuePerColour,0, 0,maxValuePerColour,maxValuePerColour, 0,0,maxValuePerColour, maxValuePerColour,0,maxValuePerColour, maxValuePerColour,maxValuePerColour,maxValuePerColour, maxValuePerColour,0,0, 0,0,0};

struct apa102LEDStrip leds;
struct colourObject dynColObject;

// SPI Vars
spi_device_handle_t spi;
spi_transaction_t spiTransObject;
esp_err_t ret;
spi_bus_config_t buscfg;
spi_device_interface_config_t devcfg;

unsigned char Colour[8][3] = { {14,5,7},{13,0,0},{14,6,0},{14,14,0},{0,5,0},{0,7,13},{3,0,9},{9,0,9}}; // sur 16 // rose // rouge // orange // jaune // vert // bleu clair // bleu foncé // mauve
unsigned char beatStepColour[3] = {13,13,5};
unsigned char stepColour[3] = {13,8,13};
unsigned char offColour[3] = {0,0,0};
unsigned char inColour[3] = {14,14,14}; // init indicator (wifi, broadcast, server address received, ready to go)
unsigned char selektorColour[10][3] = {{6,6,6},{7,7,7},{8,8,8},{9,9,9},{10,10,10},{11,11,11},{12,12,12},{13,13,13},{14,14,14},{15,15,15}}; // for feedback until knowing if it is a short or long touch
}
/////// END DELS //////

///// OSC ///////
#include <tinyosc.h> // testing for speed !?

/////// SEQ ///////

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

int step = 0;
float oldstep;

//static void periodic_timer_callback(void* arg);
//esp_timer_handle_t periodic_timer;

bool startStopCB = false; // l'état du callback 
bool startStopState = false; // l'état local

bool isPlaying = true;

double curr_beat_time;
double prev_beat_time;


/////// TIMER + button state//////

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html?highlight=hardware%20timer High Resoultion Timer API

esp_timer_handle_t oneshot_timer;

bool origButtonState; // to store button value, restore to it if we had a long press
bool currTouch; // to store if button is currently touched
bool continuousPress = false;

static void oneshot_timer_callback(void* arg)
{
    //int64_t time_since_boot = esp_timer_get_time();
    //ESP_LOGI(TAG, "Short press, there has been no touch event for over 40 ms");
    
    if ( press < 20) { // we had a short touch after all
        steps[selektor+16*barSelektor].on = !steps[selektor+16*barSelektor].on; // essaie d'écrire...espero
        
        midi[0] = midiChannel;
        midi[1] = noteSelektor;
        midi[2] = steps[selektor+16*barSelektor].on;
        midi[3] = selektor+16*barSelektor; // which step is the note info on ?
        sendData = true;
        press = 0; // reset press

        /* for ( int i = 0 ; i < 64 ; i++ ) {
            ESP_LOGI(TAG, "step %i, value : %i", i, steps[i].on);
        } */
    }

    if ( continuousPress == true ){
        // ESP_LOGI(TAG, "> 200 ms Time to register the continous press and move on");
        continuousPress = false;
        press = 0;
    }

    currTouch = false; // reset so we know
}

int oldTime = 0;
int interval = 0; 


/////////////////// WiFI station example //////////////////////
extern "C"{
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_log.h"
}

/// SMART CONFIG /// WIFI STA ///
extern "C" { 
#define EXAMPLE_ESP_MAXIMUM_RETRY  2
char ssid[33]; 
char password[65];
char str_ip[16] ="192.168.0.42"; // send IP to clients !! // stand in ip necessary for memory space?
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;
bool skipNVSRead = false; // there is a check in app_main()
bool gotIP = false; // try to get link task "tick" and udp task "socket" to wait until we have a valip IP 
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


////// LEDS ///////
extern "C"{
void renderLEDs()
{
	spi_device_queue_trans(spi, &spiTransObject, portMAX_DELAY);
}

int setupSPI()
{
	//Set up the Bus Config struct
	buscfg.miso_io_num=-1;
	buscfg.mosi_io_num=PIN_NUM_MOSI;
	buscfg.sclk_io_num=PIN_NUM_CLK;
	buscfg.quadwp_io_num=-1;
	buscfg.quadhd_io_num=-1;
	buscfg.max_transfer_sz=maxSPIFrameInBytes;
	
	//Set up the SPI Device Configuration Struct
	devcfg.clock_speed_hz=maxSPIFrequency;
	devcfg.mode=0;                        
	devcfg.spics_io_num=-1;             
	devcfg.queue_size=1;

	//Initialize the SPI driver
	ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);	
	//Add SPI port to bus
	ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);
	return ret;
}

void LEDinDicator(int lvl){
    
    for(int i = 0;i<lvl;i++){
        setPixel(&leds, i, inColour); // plus clair
	}
    renderLEDs();
	vTaskDelay(50 / portTICK_PERIOD_MS); // 10
}


void TurnLedOn(int step){  //ESP32APA102Driver

        // ESP_LOGI(TAG, "step : %i",step);
 
    	for (int i = 0; i < 16; i++){ 
			setPixel(&leds, i, offColour); // turn LED off
            if(i == step && currentBar == barSelektor){ // sélecteur d'affichage de bar
                if (steps[i+16*barSelektor].on){
                    setPixel(&leds, i, beatStepColour);    
                    }else{
                    setPixel(&leds, i, stepColour); 
                    }
                } else if (steps[i+16*barSelektor].on){
                setPixel(&leds, i, Colour[noteSelektor]);           
                }
		}
        if ( currTouch ) {
            int instaCol = int(press/2); // press has 20 values max
            setPixel(&leds, selektor, selektorColour[instaCol]); // indicate button touch state, short or continuous long we don't know yet
        }

        if ( continuousPress ) {
            setPixel(&leds, selektor, Colour[6]);
        }
        renderLEDs();
	//vTaskDelay(1 / portTICK_PERIOD_MS); // 2 better //10
    }
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

void startStopChanged(bool state) {   // received as soon as sent, we can get the state of 'isPlaying' and use that
  startStopCB = state;  // need to wait for phase to be 0 (and deal with latency...)
  ESP_LOGI(TAG, "StartStopCB : %d", startStopCB); 
}

void tickTask(void* userParam)
{

    // connect link
    ableton::Link link(120.0f);
    link.enable(true);
    link.enableStartStopSync(true); // if not no callback for start/stop
    // ESP_LOGI(TAG, "link enabled ! ");
    
    // callbacks
    // link.setTempoCallback(tempoChanged);
    link.setStartStopCallback(startStopChanged);
    
    while (true)
    { // while (true)
        xSemaphoreTake((QueueHandle_t)userParam, portMAX_DELAY);

        const auto state = link.captureAudioSessionState();
        isPlaying = state.isPlaying();
        //  ESP_LOGI(TAG, "isPlaying : , %i", isPlaying);  
        curr_beat_time = state.beatAtTime(link.clock().micros(), 4); 
        const double curr_phase = fmod(curr_beat_time, 4); 

        if (curr_beat_time > prev_beat_time && isPlaying) {

            const double prev_phase = fmod(prev_beat_time, 4);
            const double prev_step = floor(prev_phase * 4);
            const double curr_step = floor(curr_phase * 4);
            
            // https://github.com/libpd/abl_link/blob/930e8c0781b8afe9fc68321fe64c32d6e92dc113/external/abl_link~.cpp#L84
            if (abs(curr_step - prev_step) > 4 / 2 || prev_step != curr_step) {  // quantum divisé par 2

                //ESP_LOGI(TAG, "curr_step : , %f", curr_step);  
                //ESP_LOGI(TAG, "step : , %i", step);  
                //ESP_LOGI(TAG, " ");  
                step = int(curr_step);
                //ESP_LOGI(TAG, "step : , %i", step);  
                //ESP_LOGI(TAG, " ");  

                if( curr_step == 0 && isPlaying ){ 
                  
                   if(startStopCB) { // on recommence à zéro si on a reçu un message de départ
                        step = 0; // reset le compteur
                        currentBar = 0; // reset du bar
                        startStopCB = !startStopCB;
                        }    
                }   

                if(isPlaying){
                    TurnLedOn(step); // allumer les DELs 
                }
                
                if (step == 15){ //
                    if( currentBar < barSelektor ){ 
                        currentBar = currentBar + 1;
                        }
                    else{
                        currentBar = 0;
                    }
                }

            }    
        }

    prev_beat_time = curr_beat_time;

    //portYIELD();  
    vTaskDelay(2 / portTICK_PERIOD_MS); // 2 // 5 // 10 // 20 

    } // fin du while true

} // fin de tickTask


static void udp_client_task(void *pvParameters)
{

    ///// SOCKETTE TASK //////
	// upd init + timer
	dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR); //  "255.255.255.255" "192.168.0.255"
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT); // "3333"	
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;
  	inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

   
    // Trying to send a broadcast message :/
    int enabled = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));

 	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

  	if (sock < 0) {
    	ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
  		}

  	ESP_LOGI(SOCKET_TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
    ESP_LOGE(SOCKET_TAG, "sock vaut : %i", sock);

    ///// FIN SOCKETTE /////

   
    while (1) { // sendData // bdChanged est un flag si bd[] a changé

        /* for(int i = 0; i<sizeof(bd); i++){
            if (bd[i] != cbd[i]){
            ESP_LOGI(SOCKET_TAG, "bd changed 1!");
            bdChanged = true;
            break; 
            }
        } */

        if(!mstrpckIP){ 
            
            ESP_LOGI(SOCKET_TAG, "not mstrpckIP");
         
            // Sending a bogus OSC message on start, we don't have the server IP yet
            char monBuffer[16]; // monBuffer[16] // // declare a buffer for writing the OSC packet into
            //uint8_t midi[4];
            midi[0] = 42;  // midi channel // 90 + midi channel (note on)
            midi[1] = 66;  // midi note // BD // SN // LT // HT // CH // HH // CLAP // AG //
            midi[2] = 66;  // Step number? // Control Change message (11 is expression) // Pitch (note value)
            midi[3] = 66;  // Extra                                         

            int maLen = tosc_writeMessage(
                monBuffer, sizeof(monBuffer),
                "/midi", // the address
                "m",   // the format; 'f':32-bit float, 's':ascii string, 'i':32-bit integer
                midi);

            int err = sendto(sock, monBuffer, maLen, 0,(struct sockaddr *)&dest_addr, sizeof(dest_addr));
   
            if (err < 0) {
                ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
                break;
            } 
    
            ESP_LOGI(SOCKET_TAG, "Config message sent");

            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            //int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(SOCKET_TAG, "recvfrom failed : errno %d", errno);
                break;
            } else { // Data received
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                
                ESP_LOGI(SOCKET_TAG, "Received %d bytes from : %s", len, addr_str);
                ESP_LOGI(SOCKET_TAG, "tdlfServerIP : %s", rx_buffer);

                if(rx_buffer[0] == '1') { // looper number (exclusive so managed here)
                    ESP_LOGI(SOCKET_TAG, "succès UDP on ferme la sockette pour en réouvrir une autre avec la bonne adresse IP");
                    //(close socket)
                    shutdown(sock, 0);
                    close(sock);
                    mstrpckIP = true;
                    LEDinDicator(8);
                } 
            }

            vTaskDelay(100 / portTICK_PERIOD_MS); // 500 

        } // end if (!mstrpckIP)

        else if (mstrpckIP && !nouvSockette){
	        
            // upd init + timer
	        dest_addr.sin_addr.s_addr = inet_addr(rx_buffer); //  on tente vrt de changer l'adresse!!
	        dest_addr.sin_family = AF_INET;
	        dest_addr.sin_port = htons(PORT); // "3333"	
	        addr_family = AF_INET;
	        ip_protocol = IPPROTO_IP;
  	        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

         	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

            if (sock < 0) {
    	        ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
  		    }

  	        ESP_LOGI(SOCKET_TAG, "La Socket created, sending to %s:%d", rx_buffer, PORT);

            nouvSockette = true;
            LEDinDicator(16);

        } // fin de !nouvSockette

        else if ( mstrpckIP && nouvSockette ) {

            // ESP_LOGI(SOCKET_TAG, "we have a new socket and the IP of the server to send to");

            // Do this once to erase the corresponding sequence on the server side upon reboot of the client.
            // Caution, not tested, might overload things!!!
            if ( cleanSlate ){
                for ( int i = 0; i <=15;i++){
                char monBuffer[16];
                uint8_t midi[4];
                midi[0] = midiChannel;   // midi channel // 90 + midi channel (note on)
                midi[1] = noteSelektor;   // midi note // BD // SN // LT // HT // CH // HH // CLAP // AG //
                midi[2] = 0;   // 
                midi[3] = i;    // step
                int maLen = tosc_writeMessage(
                    monBuffer, sizeof(monBuffer),
                    "/midi", // the address
                    "m",   // the format; 'f':32-bit float, 's':ascii string, 'i':32-bit integer
                    midi);

                int err = sendto(sock, monBuffer, maLen, 0,(struct sockaddr *)&dest_addr, sizeof(dest_addr));
   
                if (err < 0) {
                    ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
                    break;
                } 
    
                ESP_LOGI(SOCKET_TAG, "Message sent");
                }

                cleanSlate = false;
            }

            if ( sendData ) { // This results from a new note being entered from the sequencer //

                // Testing sending OSC messages at the start
                char monBuffer[16]; // monBuffer[16] // // declare a buffer for writing the OSC packet into
                //uint8_t midi[4];
                //midi[0] = 4;   // midi channel // 90 + midi channel (note on)
                //midi[1] = 0;   // midi note // BD // SN // LT // HT // CH // HH // CLAP // AG //
                //midi[2] = 0;   // Control Change message (11 is expression) // Pitch (note value)
                //midi[3] = 0;    // Extra                                         

                int maLen = tosc_writeMessage(
                    monBuffer, sizeof(monBuffer),
                    "/midi", // the address
                    "m",   // the format; 'f':32-bit float, 's':ascii string, 'i':32-bit integer
                    midi);

                int err = sendto(sock, monBuffer, maLen, 0,(struct sockaddr *)&dest_addr, sizeof(dest_addr));
   
                if (err < 0) {
                    ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
                    break;
                } 
    
                ESP_LOGI(SOCKET_TAG, "Message sent");

                // vTaskDelay(10 / portTICK_PERIOD_MS); // 500
                
                sendData = false; // The latest change was sent

                } // Fin de 'did we have to sendData'?

                vTaskDelay(10 / portTICK_PERIOD_MS); // 500

        }  // fin mstrPuckIP && we have a nouvelle sockette 
        
    } // fin du while

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
  
    vTaskDelete(NULL); 
} // fin extern "C"


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
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED && goSMART == false) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP failed");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && goSMART == false) {  // do you things here
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        // ESP_LOGI(WIFI_TAG, "Got IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
	    esp_ip4addr_ntoa(&event->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	    ESP_LOGI(WIFI_TAG, "I have a connection and my IP is %s!", str_ip); 
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        LEDinDicator(4); 

        // udp_client // sockette
        xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
        ESP_LOGI(SOCKET_TAG, "udp_client started from IP_EVENT_STA_GOT_IP");

       /*  xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
        ESP_LOGI(SOCKET_TAG, "udp_server started from IP_EVENT_STA_GOT_IP");   */
        
        // link
        SemaphoreHandle_t tickSemphr = xSemaphoreCreateBinary();
        timerGroup0Init(1000, tickSemphr); // error: 'timerGroup0Init' was not declared in this scope
        xTaskCreate(tickTask, "tick", 8192, tickSemphr, 1, nullptr); // : error: 'timerGroup0Init' was not declared in this scope
	    ESP_LOGI(TAG, "link task started from IP_EVENT_STA_GOT_IP ");
        
        
        
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

        // or do you things here after smart config gets us an IP  

        // udp_client // sockette
        xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
        ESP_LOGI(SOCKET_TAG, "udp_client started from IP_EVENT_STA_GOT_IP from SMART CONFIG"); 

        // link timer - phase
        SemaphoreHandle_t tickSemphr = xSemaphoreCreateBinary();
        timerGroup0Init(1000, tickSemphr); // error: 'timerGroup0Init' was not declared in this scope
        xTaskCreate(tickTask, "tick", 8192, tickSemphr, 1, nullptr); // : error: 'timerGroup0Init' was not declared in this scope
	    ESP_LOGI(TAG, "link task started from IP_EVENT_STA_GOT_IP from SMART CONFIG");

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
        // ESP_LOGI(SMART_TAG, "bssid_set is true so normally we copy the credentials in memory");
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

      memcpy(ssid, evt->ssid, sizeof(evt->ssid)); 
      memcpy(password, evt->password, sizeof(evt->password));

      ESP_LOGI(SMART_TAG, "MEMCPY SSID:%s", ssid);
      ESP_LOGI(SMART_TAG, "MEMCPY PASSWORD:%s", password);

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

} // fin de extern "C"

extern "C" { static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t smtcfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&smtcfg) );
    ESP_LOGI(SMART_TAG,"normalement on a démarré le smartconfig");

    //void vTaskList(char *pcWriteBuffer)
    //vTaskSuspendAll (); // produces a vTaskDelay error

    // LED INDICATOR CODE HERE 

    while (1) {
 
      vTaskDelay(500 / portTICK_PERIOD_MS); // // 1000 
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
    
    s_wifi_event_group = xEventGroupCreate();
    s_smartcfg_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    /*if (skipNVSRead == false){
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
    }
   
    ESP_LOGI(NVS_TAG,"WIFI_STA ssid :%s",lssid); 
    ESP_LOGI(NVS_TAG,"WIFI_STA password :%s",lpassword); 
    */
    wifi_config_t wifi_config;
 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
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
        // ESP_LOGI(TAG, "connected to WiFI");

        // LEDinDicator(4); // we have wifi

    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
        wifi_config.sta.ssid, wifi_config.sta.password);
        goSMART = true; // pour la suite des choses
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
    
    }
} // fin de extern "C"


#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (90)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static bool s_pad_activated[TOUCH_PAD_MAX];
static uint32_t s_pad_init_val[TOUCH_PAD_MAX];
////// TOUCH //////

extern "C" {
////////// TOUCH ///////////
static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //read filtered value
        touch_pad_read_filtered((touch_pad_t)i, &touch_value);
        s_pad_init_val[i] = touch_value;
        ESP_LOGI(TAG, "Init: touch pad [%d] val is %d", i, touch_value);
        //set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)i, touch_value * 2 / 3));
    }
}


static void tp_example_read_task(void *pvParameter)
{
    while (1) {
       
        touch_pad_intr_enable(); //interrupt mode, enable touch interrupt
            
        for (int i = 0; i < TOUCH_PAD_MAX; i++) {

            if (s_pad_activated[i] == true) {

                interval = (esp_timer_get_time() - oldTime)/1000;  // measure time between button events
                ESP_LOGI(TAG, "Interval, %i ms", interval);
                oldTime = esp_timer_get_time();

                esp_timer_stop(oneshot_timer); // stop it if we are here 
                esp_timer_start_once(oneshot_timer, 40000); // if this triggers, we confirmed we had a short press
                
                // sendData is determined if we have a new note in 'oneshot_timer_callback'
                
                if ( interval > 350 ) {
                   press = 0;
                }
                //press = 0;
                press++; 
                ESP_LOGI(TAG, "press : %d",press);
                
                currTouch = true; // For LED indication

                if(s_pad_activated[2] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 1");
                    selektor = 0;
                    //origButtonState = bd[15+selektor+16*barSelektor]; // store value // capture piton state and return to original state if it was a long press
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[0] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 2");
                    selektor = 1;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[3] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 3");
                    selektor = 2;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    } 
                if(s_pad_activated[9] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 4");
                    selektor = 3;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    } 
                /////// 5-8
                if(s_pad_activated[2] && s_pad_activated[5]){
                    ESP_LOGI(TAG, "piton 5");
                    selektor = 4;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[0] && s_pad_activated[5]){
                    ESP_LOGI(TAG, "piton 6");
                    selektor = 5;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    } 
                if(s_pad_activated[3] && s_pad_activated[5]){
                    ESP_LOGI(TAG, "piton 7");
                    selektor = 6;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    } 
                if(s_pad_activated[9] && s_pad_activated[5]){
                    ESP_LOGI(TAG, "piton 8");
                    selektor = 7;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    } 
                /////// 9-12
                if(s_pad_activated[2] && s_pad_activated[6]){
                    ESP_LOGI(TAG, "piton 9");
                    selektor = 8;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[0] && s_pad_activated[6]){
                    ESP_LOGI(TAG, "piton 10");
                    selektor = 9;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[3] && s_pad_activated[6]){
                    ESP_LOGI(TAG, "piton 11");
                    selektor = 10;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[9] && s_pad_activated[6]){
                    ESP_LOGI(TAG, "piton 12");
                    selektor = 11;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                /////// 13-16
                if(s_pad_activated[2] && s_pad_activated[7]){
                    ESP_LOGI(TAG, "piton 13");
                    selektor = 12;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[0] && s_pad_activated[7]){
                    ESP_LOGI(TAG, "piton 14");
                    selektor = 13;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[3] && s_pad_activated[7]){
                    ESP_LOGI(TAG, "piton 15");
                    selektor = 14;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }
                if(s_pad_activated[9] && s_pad_activated[7]){
                    ESP_LOGI(TAG, "piton 16");
                    selektor = 15;
                    origButtonState = steps[selektor+16*barSelektor].on; // store value
                    }

                if(press == 20){ // number of consecutive 'presses' required for a long continuous press
                        
                        // okay we did it, feedback to the user and start a counter to prevent touches registering
                        // some bool to turn the led blue
                        
                        esp_timer_stop(oneshot_timer); // stop it if we are here 
                        continuousPress = true;
                        esp_timer_start_once(oneshot_timer, 200000); // delay after which we can reset continuous press
                        
                        ESP_LOGI(TAG, "stop the timer!!!");

                        // press = 0; // reset press

                        modSelektor = selektor;
                        ESP_LOGI(TAG, "modSelektor %d",modSelektor);
                        ESP_LOGI(TAG, " ");

                        /* if(modSelektor<8){ // change la valeur de note représentée par sa couleur
                          noteSelektor = modSelektor;  
                          ESP_LOGI(TAG, "noteSelektor %d", noteSelektor);
                          steps[0].note = noteSelektor; // Write the note value to the steps[] // Will need to write it in the substructure for up to 8 notes at the same time.
                          ESP_LOGI(TAG, "noteSelektor %i", steps[0].note);
                        }
                        */
                        /* else if(modSelektor>=8 && modSelektor<12){ // change le bar (1-16)(17-32)etc.
                           
                            // Need to know only once how many bars we have in the sequence
                            // *** Disabling the bar selektor :/ Too confusing for the space ***
                            // barSelektor = modSelektor-8;
                            barSelektor = 0;
                            // steps[0].bar = barSelektor;
                            steps[0].bar = 0;
                            //sendData = true;
                            //midi[0] = 17; // Code for a change in the bar number...changing the number of steps in the sequence (1-16)(17-32) // Implementing a smaller number of steps would be fun!
                            //midi[1] = 66;
                            //midi[2] = 66;
                            //midi[3] = 66;

                            ESP_LOGI(TAG, "Bar : %d", barSelektor);
                        } */

                        /* else if( modSelektor == 12){ 
                            if (midiChannel == 7){ // le max
                                midiChannel = -1; // loop it
                                }
                            // *** Disabling midi channel for Banshees, too confusing, notes are sufficient for the solenoids.
                            // midiChannel = midiChannel+1 ; // à considérer un 'enter' pour rendre les modifs actives à ce moment uniquement
                            // Need to set the midi channel only once for the sequence
                            //steps[0].chan = midiChannel; // Write the channel number 
                            ESP_LOGI(TAG, "Midi channel %i", midiChannel);
                        } */
                        
                        /* else if( modSelektor == 13 ){ // Won't change note duration for Banshees
                            if (noteDuration == 7) {
                                noteDuration = -1;
                            }
                            noteDuration = noteDuration + 1;
                            // Need to set the note length only once for the sequence
                            steps[0].length = noteDuration;
                            ESP_LOGI(TAG, "noteDuration : %i", steps[0].length); 
                        }
 */
                        /* else if( modSelektor == 14 ) {
                            // Need to set the mute info only once for the sequence // Are we playing or not?
                            steps[0].mute = !steps[0].mute;
                            midi[0] = 18; // Channel (up to 16) so 18 is code for muting that track
                            midi[1] = 66;
                            midi[2] = 66;
                            midi[3] = 66;
                            // sendData = true;
                            ESP_LOGI(TAG, "mute : %i", steps[0].mute); 
                        } */
                        
                        /* else if( modSelektor == 15 ) { // reset values
                            // Erase all values from the sequencer
                            for (i = 0 ; i < 64 ; i++) {
                                steps[i].on = 0;
                                steps[i].chan = 0;
                                // steps[i].note = 0;
                                steps[i].length = 0;
                                steps[i].mute = 0;
                            }

                            midi[0] = 19; // Code for reset on the server
                            midi[1] = 66; // no such note
                            midi[2] = 66; // no such bar
                            midi[3] = 66; 
                            sendData = true; // TODO : Ajouter l'envoi d'un message de reset du côté de tdlf // We need to send a reset message to the server
                            modSelektor = 42; // ok on arrête d'effacer...
                            ESP_LOGI(TAG, "RESET");
                        } */

                    }

                    vTaskDelay(20 / portTICK_PERIOD_MS); // 100 Wait a while for the pad being released
                    s_pad_activated[i] = false; // Clear information on pad activation
                    // show_message = 1;  /// // Reset the counter triggering a message that application is running
                }
            }

        vTaskDelay(10 / portTICK_PERIOD_MS); // 10 since 5 triggers watchdog

    }
}


static void tp_example_rtc_intr(void *arg)
{
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}


static void tp_example_touch_pad_init(void) // Before reading touch pad, we need to initialize the RTC IO.
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config((touch_pad_t)i, TOUCH_THRESH_NO_USE);
    }
}

///////// FIN TOUCH //////////


} // fin de extern "C"


unsigned int if_nametoindex(const char* ifname)
{
  return 0;
}

char* if_indextoname(unsigned int ifindex, char* ifname)
{
  return nullptr;
}


extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();   //Initialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /// Error check NVS credentials ///

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
      //////// WRITING TO NVS // 
      nvs_handle wificfg_nvs_handler;
      nvs_open("Wifi", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_set_str(wificfg_nvs_handler,"wifi_ssid","testing");
      nvs_set_str(wificfg_nvs_handler,"wifi_password","onetwo");
      nvs_commit(wificfg_nvs_handler); 
      nvs_close(wificfg_nvs_handler); 
      ////// END NVS ///// 
    }

    //// LEDS /////
    printf("\r\n\r\nHello Pixels!\n");
	//Set up SPI
	printf("Setting up SPI now\t[%d]\r\n", setupSPI());
	//set up LED object
	printf("Creating led object...\t");
	initLEDs(&leds, totalPixels, bytesPerPixel, 255); // 255
	printf("Frame Length\t%d\r\n", leds._frameLength);
	//set up colours
	initComplexColourObject(&dynColObject, maxValuePerColour, 9, colourList);	
	//Set up SPI tx/rx storage Object
	memset(&spiTransObject, 0, sizeof(spiTransObject));
	spiTransObject.length = leds._frameLength*8;
	spiTransObject.tx_buffer = leds.LEDs;
	printf("SPI Object Initialized...\r\n");
	printf("Sending SPI data block to clear all pixels....\r\n");
	spi_device_queue_trans(spi, &spiTransObject, portMAX_DELAY);
	printf("Pixels Cleared!\r\n");
    //// LEDS /////

    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    if (goSMART == true){
        //ESP_ERROR_CHECK(esp_wifi_stop());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //ESP_LOGI(WIFI_TAG, "STOP STA");
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, &xHandle);
    }

  	//esp_wifi_set_ps(WIFI_PS_NONE);
	
	/////// TOUCH INIT ////////
	ESP_LOGI(TAG, "Initializing touch pad");
	touch_pad_init(); // Initialize touch pad peripheral, it will start a timer to run a filter
	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V); // Set reference voltage for charging/discharging // For most usage scenarios, we recommend using the following combination: // the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
	tp_example_touch_pad_init(); // Init touch pad IO
	touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD); // Initialize and start a software filter to detect slight change of capacitance.
	tp_example_set_thresholds(); // Set thresh hold
	touch_pad_isr_register(tp_example_rtc_intr, NULL); // Register touch interrupt ISR

	////////// TOUCH TASK //////////// // Start a task to show what pads have been touched
	xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL); // n'importe quelle core
	///////// FIN TOUCH /////////

    // link + udp task are started within the event handler as we need to wait for IP_EVENT_STA_GOT_IP
    
   

    // timers ??
  
        const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &oneshot_timer_callback,
        // argument specified here will be passed to timer callback function 
       .arg = NULL,
        //.dispatch_method = ESP_TIMER_TASK,
        .name = "one-shot"
    };

    esp_timer_create(&oneshot_timer_args, &oneshot_timer);
    //esp_timer_start_once(oneshot_timer, 5000000);
    // end timers ?

     vTaskDelete(nullptr);

} // fin du extern "C"