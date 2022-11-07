//touch_pad_2 (GPIO_NUM_2), touch_pad_3 (GPIO_NUM_15), touch_pad_4 (GPIO_NUM_14), touch_pad_5 (GPIO_NUM_12), touch_pad_7 (GPIO_NUM_27), touch_pad_9 (GPIO_NUM_32)

extern "C" {
#include "driver/touch_pad.h" 
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"
static const char *TOUCH_TAG = "Touch pad";
static const char *TAP_TAG = "TAP";

//#define TOUCH_PAD_NO_CHANGE   (-1) // not necessary ?
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80) // 95
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10) // 10

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
//bool seqToLoad = false; // to know when to send the loaded sequence to clients
bool loadSeqConf = false;
bool saveDelay = false; // for when to remove the save options after an interaction
int delset = 0;
int selectedSeq = 0; // user selected sequence to load
bool rando = false; // Adding a random option to filter out notes
int nmbrSeq = 0; // the number of sequences in nvs
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

bool toTapped = false;
// bool startStopState = false; // l'état local
bool changePiton = false; 
bool tempoINC = false; // si le tempo doit être augmenté
bool tempoDEC = false; // si le tempo doit être réduit
 

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
          rando = !rando; // Toggle random values
          // loadSeq = true;
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
