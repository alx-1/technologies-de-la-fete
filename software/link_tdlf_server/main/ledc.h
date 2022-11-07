#include "driver/ledc.h" // For CV Pitch 
#include "esp_err.h"

// CV Outputs //
#define CV_18  (GPIO_NUM_18) 
#define CV_5  (GPIO_NUM_5) 
#define GPIO_OUTPUT_PIN_SEL  ( (1ULL<<CV_18) | (1ULL<<CV_5) )
int CV_TIMING_CLOCK = 0;
int CV_TRIGGER_OFF[16] = {0}; 

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

} 