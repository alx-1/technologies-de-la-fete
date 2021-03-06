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

#define LED GPIO_NUM_2
#define PRINT_LINK_STATE false
#define USE_TOUCH_PADS // touch_pad_7 (GPIO_NUM_27), touch_pad_8 (GPIO_NUM_33)
#define USE_START_STOP // GPIO_NUM_16
#define USE_I2C_DISPLAY // SDA GPIO_NUM21, SCL GPIO_NUM_22


// Serial midi
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_5)
#define BUF_SIZE (1024)
#define MIDI_TIMING_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP 0xFC
#define MIDI_SONG_POSITION_POINTER 0xF2

#if defined USE_TOUCH_PADS
extern "C" {
#include "freertos/queue.h"
#include "esp_log.h" 
#include "driver/touch_pad.h" 
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

static const char *TAG = "Touch pad";

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static bool s_pad_activated[2];
static uint32_t s_pad_init_val[2];

// Global
static void periodic_timer_callback(void* arg);
esp_timer_handle_t periodic_timer;
bool startStopCB = false;
bool startStopState = false;
double curr_beat_time;
double prev_beat_time;

static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;
     for (int i = 7; i < 9; i++) {
        touch_pad_read_filtered((touch_pad_t)i, &touch_value);
        s_pad_init_val[i] = touch_value;
        ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", i, touch_value); //set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh((touch_pad_t)i, touch_value * 2 / 3));
     }
}
static void tp_example_read_task(void *pvParameter) {
    
    static int show_message;
    
 while (1) {
     
    touch_pad_intr_enable();
    for (int i = 7; i < 9; i++) {
        if (s_pad_activated[i] == true) {
        ESP_LOGI(TAG, "T%d activated!", i);  // Wait a while for the pad being released
        vTaskDelay(200 / portTICK_PERIOD_MS);  // Clear information on pad activation
        s_pad_activated[i] = false; // Reset the counter triggering a message // that application is running
        show_message = 1;
        }
    }
        
    vTaskDelay(10 / portTICK_PERIOD_MS);
     
    }
}


static void tp_example_rtc_intr(void *arg) { //  Handle an interrupt triggered when a pad is touched. Recognize what pad has been touched and save it in a table.
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = 7; i < 9; i++) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}


static void tp_example_touch_pad_init(void) { // Before reading touch pad, we need to initialize the RTC IO.
    for (int i = 7; i < 9; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config((touch_pad_t)i, TOUCH_THRESH_NO_USE);
    }
}

}    
#endif


#if defined USE_START_STOP
extern "C" {
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

#define GPIO_INPUT_IO_0     GPIO_NUM_16
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)  
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level((gpio_num_t)io_num));
        }
    }
}
}
#endif

#if defined USE_I2C_DISPLAY
#define PRINT_LINK_STATE true  // je vais changer ceci pour afficher uniquement lorsqu'il y a un changement de BPM
extern "C" {
#include <stdbool.h>
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"


static const int I2CDisplayAddress = 0x3C;
static const int I2CDisplayWidth = 128;
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
#endif

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

void printTask(void* userParam)
{
  auto link = static_cast<ableton::Link*>(userParam);
  const auto quantum = 4.0;
    
#if defined USE_I2C_DISPLAY   
  if ( DefaultBusInit( ) == true ) {
        printf( "BUS Init lookin good...\n" );
        printf( "Drawing.\n" );
        SetupDemo( &I2CDisplay, &Font_droid_sans_mono_13x24 );
        SayHello( &I2CDisplay, "Link!" );
        printf( "Done!\n" );
   }
#endif  

  while (true)
  {
    const auto sessionState = link->captureAppSessionState();
    const auto numPeers = link->numPeers();
    const auto time = link->clock().micros();
    const auto beats = sessionState.beatAtTime(time, quantum);
      
#if defined USE_I2C_DISPLAY
    char buf[10];
    snprintf(buf, 10 , "%i", (int) round( sessionState.tempo() ) );
#endif
      
    std::cout << std::defaultfloat << "| peers: " << numPeers << " | "
              << "tempo: " << sessionState.tempo() << " | " << std::fixed
              << "beats: " << beats << " |" << std::endl;
    vTaskDelay(800 / portTICK_PERIOD_MS);
    
#if defined USE_I2C_DISPLAY
      SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );
      SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24);
      SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, "BPM", SSD_COLOR_WHITE );
      SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, buf, SSD_COLOR_WHITE );
      SSD1306_Update( &I2CDisplay );   
#endif
      
  }
}

// callbacks
void tempoChanged(double tempo) {
    double midiClockMicroSecond = ((60000 / tempo) / 24) * 1000;
    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) periodic_timer;
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, midiClockMicroSecond));
}

void startStopChanged(bool isPlaying) {
  // received as soon as sent
  // need to wait for phase to be 0 (and deal with latency...)
  std::cout << std::defaultfloat << "| state: " << isPlaying << std::endl;
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

  // debug
  if (PRINT_LINK_STATE)
  {
    xTaskCreate(printTask, "print", 8192, &link, 1, nullptr);
  }
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);

  // phase
  while (true)
  { 
    xSemaphoreTake(userParam, portMAX_DELAY);

    const auto state = link.captureAudioSessionState();
    const auto phase = state.phaseAtTime(link.clock().micros(), 1.);
    gpio_set_level(LED, fmodf(phase, 1.) < 0.1);

    // start / stop midi
    curr_beat_time = state.beatAtTime(link.clock().micros(), 4);
    const double curr_phase = fmod(curr_beat_time, 4);
    if (curr_beat_time > prev_beat_time) {
      const double prev_phase = fmod(prev_beat_time, 4);
      const double prev_step = floor(prev_phase * 1);
      const double curr_step = floor(curr_phase * 1);
      if (prev_phase - curr_phase > 4 / 2 || prev_step != curr_step) {
        if(curr_step == 0 && startStopState != startStopCB) {
              if(startStopCB) {
                char zedata[] = { MIDI_START };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                uart_write_bytes(UART_NUM_1, 0, 1);
              } else {
                char zedata[] = { MIDI_STOP };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                uart_write_bytes(UART_NUM_1, 0, 1);
              }
              startStopState = startStopCB;
        }
      }
    }
    prev_beat_time = curr_beat_time;

    portYIELD();
  }
}

static void periodic_timer_callback(void* arg)
{
    char zedata[] = { MIDI_TIMING_CLOCK };
    uart_write_bytes(UART_NUM_1, zedata, 1);
}

extern "C" void app_main()
{
  ESP_ERROR_CHECK(nvs_flash_init());
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect());

  // UART
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

  // SOFTWARE TIMER - LINK = PHASE & LED
  SemaphoreHandle_t tickSemphr = xSemaphoreCreateBinary();
  timerGroup0Init(1000, tickSemphr);
  xTaskCreate(tickTask, "tick", 8192, tickSemphr, 1, nullptr);

  // HARDWARE TIMER - MIDI CLOCK
  const esp_timer_create_args_t periodic_timer_args = {
          .callback = &periodic_timer_callback,
          .name = "periodic"
  };
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 20833.333333333)); // 120bpm by default
    
  #if defined USE_TOUCH_PADS
    ESP_LOGI(TAG, "Initializing touch pad");     // Initialize touch pad peripheral, it will start a timer to run a filter
    touch_pad_init();
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);   // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);// Set reference voltage for charging/discharging
    tp_example_touch_pad_init();    // Init touch pad IO
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD); // Initialize and start a software filter to detect slight change of capacitance.
    tp_example_set_thresholds();     // Set thresh hold
    touch_pad_isr_register(tp_example_rtc_intr, NULL); // Register touch interrupt ISR
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL); // Start a task to show what pads have been touched
  #endif
  
  #if defined USE_START_STOP
    gpio_config_t io_conf;   // GPIO
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_NEGEDGE; 
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE; //disable interrupt
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;  //bit mask of the pins, use GPIO4
    io_conf.mode = GPIO_MODE_INPUT;  //set as input mode   
    io_conf.pull_down_en = (gpio_pulldown_t)0; // 0 //disable pull-down mode
    io_conf.pull_up_en = (gpio_pullup_t)1; //enable pull-up mode
    gpio_config(&io_conf);

    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE); // GPIO_INTR_ANYEDGE //change gpio interrupt type for one pin
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t)); //create a queue to handle gpio event from isr
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL); //start gpio task

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT); //install gpio isr service
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0); //hook isr handler for specific gpio pin
    gpio_isr_handler_remove(GPIO_INPUT_IO_0); //remove isr handler for gpio number. 
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);  //hook isr handler for specific gpio pin again
  #endif
}
