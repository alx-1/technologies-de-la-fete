//// tdlf client // 16 touch pads // 16 leds // envoi des données de un array au client
//// dois ajouter une sélection automatique du point d'accès lorsque l'IP change

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

//#define HOST_IP_ADDR "192.168.0.101" // synth is using (will use) a static ip

#define HOST_IP_ADDR "255.255.255.255" // trying to broadcast first
#define PORT 3333
// #define SO_BROADCAST   0x0020

////// sockette à rajouter /////

char rx_buffer[128]; // 128
char addr_str[128];
int addr_family;    
int ip_protocol;
struct sockaddr_in dest_addr;
int sock;
bool mstrpckIP = false;
bool nouvSockette = false;
bool bdChanged = false;


int press;
int selektor; // couleur rose par défaut
int modSelektor = 0;
int fourChan = 0; // channel à stocker ds les premiers 0-3 bits de bd[]
int noteSelektor = 0; // note à stocker dans les bits 4-7 de bd[]
int barSelektor = 0; // à stocker
int currentBar = 0; // pour l'affichage


///////// DELS // SPI CONFIG TTGO // VSPI // SPI3 // MOSI 23 // SCK 18
extern "C"{
#include "ESP32APA102Driver.h"

unsigned char colourList[9*3]={maxValuePerColour,0,0, maxValuePerColour,maxValuePerColour,0, 0,maxValuePerColour,0, 0,maxValuePerColour,maxValuePerColour, 0,0,maxValuePerColour, maxValuePerColour,0,maxValuePerColour, maxValuePerColour,maxValuePerColour,maxValuePerColour, maxValuePerColour,0,0, 0,0,0};

struct apa102LEDStrip leds;
struct colourObject dynColObject;

//SPI Vars
spi_device_handle_t spi;
spi_transaction_t spiTransObject;
esp_err_t ret;
spi_bus_config_t buscfg;
spi_device_interface_config_t devcfg;

unsigned char Colour[8][3] = { {16,7,9},{13,0,0},{14,6,0},{14,14,0},{0,5,0},{0,7,13},{3,0,9},{9,0,9}}; // sur 16 // rose // rouge // orange // jaune // vert // bleu clair // bleu foncé // mauve
unsigned char beatStepColour[3] = {13,13,5};
unsigned char stepColour[3] = {11,4,11};
unsigned char offColour[3] = {0,0,0};
unsigned char inColour[3] = {14,14,14}; // init indicator (wifi, broadcast, server address received, ready to go)
}
/////// DELS //////


/////// SEQ ///////

bool bd[72] = {}; // empty 16 boolean values aray to store hits // 4 bits channel, 4 bits note // 64 bit notes
bool cbd[72] = {1}; // to determine if we have a changed bd
int step = 0;
float oldstep;

static void periodic_timer_callback(void* arg);
esp_timer_handle_t periodic_timer;
// 
bool startStopCB = false; // l'état du callback 
bool startStopState = false; // l'état local

bool isPlaying = true;

double curr_beat_time;
double prev_beat_time;

/////////////////// WiFI station example //////////////////////
extern "C"{
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
//#include "nvs_flash.h"
//#include "lwip/err.h"
//#include "lwip/sys.h"
}

extern "C" { 
#define EXAMPLE_ESP_WIFI_SSID      "link"
#define EXAMPLE_ESP_WIFI_PASS      "nidieunimaitre"
#define EXAMPLE_ESP_MAXIMUM_RETRY  2

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG = "tdlf client";

static int s_retry_num = 0;
}

extern "C" { 
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
                                {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        // LEDinDicator(3); // grr not declared in this scope
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
} // fin de extern "C"

extern "C" { void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    
    ////
    wifi_config_t wifi_config = { // whatever parce que C, c'est pas C++
      { EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS }
      };

    /////
 
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
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
        EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        //LEDinDicator(4); // we have wifi

    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
}
} // fin de extern "C"


///// fin station example

// static const char *TAG = "Touch pad";

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (90)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static bool s_pad_activated[TOUCH_PAD_MAX];
static uint32_t s_pad_init_val[TOUCH_PAD_MAX];
////////////// TOUCH /////////


extern "C" {

void LEDinDicator(int lvl){
    
    for(int i = 0;i<lvl;i++){
        setPixel(&leds, i, inColour); // plus clair
	}

    renderLEDs();

	vTaskDelay(50 / portTICK_PERIOD_MS); // 10

}

void TurnLedOn(int step){  //ESP32APA102Driver
	
 
    	for (int i = 0; i < 16; i++){ 

			setPixel(&leds, i, offColour); // turn LED off

            if(i == step && currentBar == barSelektor){ // sélecteur d'affichage de bar
                if (bd[i+8+16*barSelektor]){
                    setPixel(&leds, i, beatStepColour);    
                    }else{
                    setPixel(&leds, i, stepColour); 
                    }
            }else if (bd[i+8+16*barSelektor] && noteSelektor == 0){
                setPixel(&leds, i, Colour[0]); // rose
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 1){
                setPixel(&leds, i, Colour[1]); // rouge
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 2){
                setPixel(&leds, i, Colour[2]); // orange
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 3){
                setPixel(&leds, i, Colour[3]); // jaune
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 4){
                setPixel(&leds, i, Colour[4]); // vert
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 5){
                setPixel(&leds, i, Colour[5]); // bleu clair
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 6){
                setPixel(&leds, i, Colour[6]); // bleu
                }else if (bd[i+8+16*barSelektor] && noteSelektor == 7){
                setPixel(&leds, i, Colour[7]); // mauve
                }

		}

        renderLEDs();

	vTaskDelay(100 / portTICK_PERIOD_MS); // 10

}

void convertInt2Bits(int monInt, int monOffset){
    // monInt à convertir, monOffset pour l'écrire au bon endroit

    for(int i=0;i<4;i++){ // reset avant de ré-écrire les valeurs
        bd[i+4*monOffset]= false;
    }
    
    if(monInt == 0){
        // do nothing
        }
        else if( monInt == 1 ){ bd[3+4*monOffset] = true; }
        else if( monInt == 2 ){ bd[2+4*monOffset] = true; }
        else if( monInt == 3 ){ bd[3+4*monOffset] = true; bd[2+4*monOffset] = true; }
        else if( monInt == 4 ){ bd[1+4*monOffset] = true; }
        else if( monInt == 5 ){ bd[3+4*monOffset] = true; bd[1+4*monOffset] = true; }
        else if( monInt == 6 ){ bd[2+4*monOffset] = true; bd[1+4*monOffset] = true; }
        else if( monInt == 7 ){ bd[3+4*monOffset] = true; bd[2+4*monOffset] = true; bd[1+4*monOffset] = true; } 
        else{
        ESP_LOGI(TAG, "monInt out of range");
        }
        ESP_LOGI(TAG, "noteSelektor %i", monInt);
    } // fin converter

////////// UDP SOCKETTE ////////

static void udp_client_task(void *pvParameters)
{
    while (1) {

        while (1) { // bdChanged est un flag si bd[] a changé

            for(int i = 0; i<sizeof(bd); i++){
                    if (bd[i] != cbd[i]){
                        ESP_LOGE(TAG, "bd changed!");
                        bdChanged = true;
                    }

            }

            if(!mstrpckIP){ 
            int err = sendto(sock, bd, sizeof(bd), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            ESP_LOGI(TAG, "Message sent");

            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            //int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            } else { // Data received
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);

                ESP_LOGI(TAG, "mstrpckIP : ");
                //HOST_IP_ADDR = rx_buffer;
                ESP_LOGI(TAG, "%s", rx_buffer);

                if(rx_buffer[0] == '1') { // looper number (exclusive so managed here)
                    ESP_LOGI(TAG, "succès UDP on ferme la sockette pour en réouvrir une autre avec la bonne adresse IP");
                    //(close socket)
                    shutdown(sock, 0);
                    close(sock);
                    mstrpckIP = true;
                    LEDinDicator(8);
                } 
            }

            vTaskDelay(500 / portTICK_PERIOD_MS);

        } // end if (mstrpck)

        else if (mstrpckIP && !nouvSockette){
            ///// SOCKETTE TASK //////
	        // upd init + timer

	        dest_addr.sin_addr.s_addr = inet_addr(rx_buffer); //  on tente vrt de changer l'adresse!!
	        dest_addr.sin_family = AF_INET;
	        dest_addr.sin_port = htons(PORT); // "3333"	
	        addr_family = AF_INET;
	        ip_protocol = IPPROTO_IP;
  	        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

         	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

            if (sock < 0) {
    	        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
  		    }

  	        ESP_LOGI(TAG, "Socket created, sending to %s:%d", rx_buffer, PORT);

            nouvSockette = true;
            LEDinDicator(16);

	        ///// FIN SOCKETTE /////

        } // fin de !nouvSockette

        else if (mstrpckIP && nouvSockette && bdChanged){

            ESP_LOGI(TAG, "tente d'envoyer");
           int err = sendto(sock, bd, sizeof(bd), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

             for(int i = 0; i<sizeof(bd); i++){
                    cbd[i] = bd[i];
                    }

            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                 break;
            }
            
            ESP_LOGI(TAG, "Message sent");

            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            //int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            } else { // Data received
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);

                ESP_LOGI(TAG, "2e confirmation mstrpckIP  : ");
                //HOST_IP_ADDR = rx_buffer;
                ESP_LOGI(TAG, "%s", rx_buffer); 

                if(rx_buffer[0] == '1') { // looper number (exclusive so managed here)
                    ESP_LOGI(TAG, "succès UDP");
                } 

            }
            vTaskDelay(500 / portTICK_PERIOD_MS);

            bdChanged = false;
        } // fin nouvelle sockette

        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL); 
}


////////// TOUCH ///////////
static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //read filtered value
        touch_pad_read_filtered((touch_pad_t)i, &touch_value);
        s_pad_init_val[i] = touch_value;
        ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", i, touch_value);
        //set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)i, touch_value * 2 / 3));
    }
}


static void tp_example_read_task(void *pvParameter)
{
    static int show_message;
    int change_mode = 0;
    int filter_mode = 0;

    while (1) {
        if (filter_mode == 0) {  
            touch_pad_intr_enable(); //interrupt mode, enable touch interrupt
            
            for (int i = 0; i < TOUCH_PAD_MAX; i++) {
                if (s_pad_activated[i] == true) {

                /// logique ici
                if(s_pad_activated[2] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 1");
                    selektor = 0;
                    bd[8+0+16*barSelektor] = !bd[8+0+16*barSelektor]; // 8 est le offset ds le tableau
                        }
                if(s_pad_activated[0] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 2");
                    selektor = 1;
                    bd[8+1+16*barSelektor] = !bd[8+1+16*barSelektor];
                        }
                if(s_pad_activated[3] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 3");
                    selektor = 2;
                    bd[8+2+16*barSelektor] = !bd[8+2+16*barSelektor];
                        }
                if(s_pad_activated[9] && s_pad_activated[4]){
                    ESP_LOGI(TAG, "piton 4");
                    selektor = 3;
                    bd[8+3+16*barSelektor] = !bd[8+3+16*barSelektor];
                        }
                /////// 5-8
                if(s_pad_activated[2] && s_pad_activated[5]){
                            ESP_LOGI(TAG, "piton 5");
                            selektor = 4;
                            bd[8+4+16*barSelektor] = !bd[8+4+16*barSelektor];
                                }
                if(s_pad_activated[0] && s_pad_activated[5]){
                            ESP_LOGI(TAG, "piton 6");
                            selektor = 5;
                            bd[8+5+16*barSelektor] = !bd[8+5+16*barSelektor];
                                }
                if(s_pad_activated[3] && s_pad_activated[5]){
                            ESP_LOGI(TAG, "piton 7");
                            selektor = 6;
                            bd[8+6+16*barSelektor] = !bd[8+6+16*barSelektor];
                                }
                if(s_pad_activated[9] && s_pad_activated[5]){
                            ESP_LOGI(TAG, "piton 8");
                            selektor = 7;
                            bd[8+7+16*barSelektor] = !bd[8+7+16*barSelektor];
                                }
                /////// 9-12
                if(s_pad_activated[2] && s_pad_activated[6]){
                        ESP_LOGI(TAG, "piton 9");
                        selektor = 8;
                        bd[8+8+16*barSelektor] = !bd[8+8+16*barSelektor];
                            }
                if(s_pad_activated[0] && s_pad_activated[6]){
                        ESP_LOGI(TAG, "piton 10");
                        selektor = 9;
                        bd[8+9+16*barSelektor] = !bd[8+9+16*barSelektor];
                            }
                if(s_pad_activated[3] && s_pad_activated[6]){
                        ESP_LOGI(TAG, "piton 11");
                        selektor = 10;
                        bd[8+10+16*barSelektor] = !bd[8+10+16*barSelektor];
                            }
                if(s_pad_activated[9] && s_pad_activated[6]){
                        ESP_LOGI(TAG, "piton 12");
                        selektor = 11;
                        bd[8+11+16*barSelektor] = !bd[8+11+16*barSelektor];
                            }
                /////// 13-16
                if(s_pad_activated[2] && s_pad_activated[7]){
                      ESP_LOGI(TAG, "piton 13");
                      selektor = 12;
                      bd[8+12+16*barSelektor] = !bd[8+12+16*barSelektor];
                          }
                if(s_pad_activated[0] && s_pad_activated[7]){
                      ESP_LOGI(TAG, "piton 14");
                      selektor = 13;
                      bd[8+13+16*barSelektor] = !bd[8+13+16*barSelektor];
                          }
                if(s_pad_activated[3] && s_pad_activated[7]){
                      ESP_LOGI(TAG, "piton 15");
                      selektor = 14;
                      bd[8+14+16*barSelektor] = !bd[8+14+16*barSelektor];
                          }
                if(s_pad_activated[9] && s_pad_activated[7]){
                      ESP_LOGI(TAG, "piton 16");
                      selektor = 15;
                      bd[8+15+16*barSelektor] = !bd[8+15+16*barSelektor];
                          }

                    // ESP_LOGI(TAG, "T%d activated!", i);
					// genre de timer pour détecter les longs press// 
                    press++;

                    if(press >= 50){ // pause

                        modSelektor = selektor;

                        ESP_LOGI(TAG, "modSelektor %d",modSelektor);

                        if(modSelektor<8){ // change la valeur de note représentée par sa couleur
                          noteSelektor = modSelektor;  
                          ESP_LOGI(TAG, "noteSelektor %d", noteSelektor);
                          convertInt2Bits(noteSelektor, 1); // écrit les valeurs de note en bits ds bd[]
                        }

                        else if(modSelektor>=8 && modSelektor<12){ // change le bar (1-16)(17-32)etc.
                            barSelektor= modSelektor-8;
                            ESP_LOGI(TAG, "barSelektor %d", barSelektor);
                        }

                        else if( modSelektor == 13){ 
                            if (fourChan == 7){ // le max
                                fourChan = 0; // loop it
                            }
                            fourChan = fourChan+1 ; // à considérer un 'enter' pour rendre les modifs actives à ce moment uniquement
                            convertInt2Bits(fourChan, 0); // écrit les valeurs de channel en bits ds bd[]
                            ESP_LOGI(TAG, "Channel %i", fourChan);
                        }
                        
                        else if( modSelektor == 14){
                            if (fourChan == 0){ // le min
                                fourChan = 7; // loop it
                            }
                            fourChan = fourChan-1;
                            convertInt2Bits(fourChan, 0); // écrit les valeurs de channel en bits ds bd[]
                            ESP_LOGI(TAG, "Channel %i", fourChan);  
                        }
                        
                        else if(modSelektor == 15){ // reset beat values
                            size_t n = sizeof(bd)/sizeof(bd[0]);
                            for(i=0;i<n;i++){ // reset tout
                                bd[i]=0;  
                            }
                            // ajouter un reset du côté de tdlf
                            modSelektor = 42; // ok on arrête d'effacer...
                            ESP_LOGI(TAG, "RESET");
                        }

                        press = 0;
                    }
                    
                    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait a while for the pad being released
                    s_pad_activated[i] = false; // Clear information on pad activation
                    show_message = 1;  /// // Reset the counter triggering a message that application is running
                }
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);

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
/*
void tempoChanged(double tempo) {
    ESP_LOGI(TAG, "tempochanged");
    double midiClockMicroSecond = ((60000 / tempo) / 24) * 1000;
    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) periodic_timer;
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, midiClockMicroSecond));
// }
*/

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
    // ESP_LOGI(TAG, "on se rend ici link enabled ! ");
    
    // callbacks
    // link.setTempoCallback(tempoChanged);
    link.setStartStopCallback(startStopChanged);
    
    while (true)
    {
        xSemaphoreTake(userParam, portMAX_DELAY);

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

                if( curr_step == 0 && isPlaying ){ 
                    
                    if(startStopCB) { // on recommence à zéro si on a reçu un message de départ
                        step = 0; // reset le compteur
                        currentBar = 0; // reset du bar
                        startStopCB = !startStopCB;
                    }    
                }   
                //ESP_LOGI(TAG, "step : %d", step);  
                
                if(isPlaying){
                    // ESP_LOGI(TAG, "step : %i", step);
                    TurnLedOn(step); // allumer les DELs
                    step++; 
                }
                
                if (step == 16){ // il n'y a que 16 DELs
                    step = 0;
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

    portYIELD(); // --- > vTaskDelay(20 / portTICK_PERIOD_MS);
 
    } // fin du while true

} // fin de tickTask


extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();   //Initialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

  	//esp_wifi_set_ps(WIFI_PS_NONE);

	// LEDSTRIP.init(16); // arg est le nombre de LEDs

    //// LEDS /////
    printf("\r\n\r\n\r\nHello Pixels!\n");
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
	printf("SPI Object Initilized...\r\n");
	printf("Sending SPI data block to clear all pixels....\r\n");
	spi_device_queue_trans(spi, &spiTransObject, portMAX_DELAY);
	printf("Pixels Cleared!\r\n");
    //// LEDS /////
	

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

	
	///// SOCKETTE TASK //////
	// upd init + timer

	dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR); //  "192.168.0.255"
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT); // "3333"	
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;
  	inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    // Trying to send a broadcast message, not sure it works :/
    int enabled = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));

 	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);


  	if (sock < 0) {
    	ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
  		}

  	ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);

	xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
	///// FIN SOCKETTE /////
	
	// ---> TaskCreatePinnedToCore(link_task, "link_task", 8192, nullptr, 10, nullptr, 0);  // core 0
	
     // link timer - phase
    SemaphoreHandle_t tickSemphr = xSemaphoreCreateBinary();
    timerGroup0Init(1000, tickSemphr);
    xTaskCreate(tickTask, "tick", 8192, tickSemphr, 1, nullptr);

	ESP_LOGI(TAG, "on se rend ici, après link task ! ");

	vTaskDelete(nullptr);

} // fin du extern "C"

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
}
