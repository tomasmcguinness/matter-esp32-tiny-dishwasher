#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"
//#include "1602_lcd.h"
#include "esp_lcd.h"

#define RED_GPIO 0
#define YELLOW_GPIO 1
#define GREEN_GPIO 2

static const char *TAG = "status_display";

StatusDisplay StatusDisplay::sStatusDisplay;

esp_err_t StatusDisplay::Init()
{
    gpio_reset_pin((gpio_num_t)RED_GPIO);
    gpio_set_direction((gpio_num_t)RED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)RED_GPIO, 0);

    gpio_reset_pin((gpio_num_t)YELLOW_GPIO);
    gpio_set_direction((gpio_num_t)YELLOW_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)YELLOW_GPIO, 0);

    gpio_reset_pin((gpio_num_t)GREEN_GPIO);
    gpio_set_direction((gpio_num_t)GREEN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)GREEN_GPIO, 0);

    lcd_t lcd;

    /* Set default pinout */
    lcdDefault(&lcd);

    lcdInit(&lcd);

  /* Clear previous data on LCD */
  lcdClear(&lcd);

  /* Custom text */
  char buffer[16] = "E";
  float version = 1.0f;
  char initial[2] = {'J', 'M'};
  //sprintf(buffer, "ESP v%.1f %c%c", version, initial[0], initial[1]);

  /* Set text */

  lcd_err_t ret = lcdSetText(&lcd, buffer, 0, 0);

  /* Check lcd status */
  assert_lcd(ret);

    //lcd_err_t ret = LCD_OK;

    // Initialize LCD object
    //lcdInit();

    // Clear previous data on LCD 
    //ret = lcdClear();
    //lcdAssert(ret);

    return ESP_OK;
}

void StatusDisplay::SetStopped()
{
    //lcd_err_t ret = lcdClear();
    //lcdAssert(ret);

    ESP_LOGI(TAG, "Setting status to stopped");
    gpio_set_level((gpio_num_t)RED_GPIO, 1);
    gpio_set_level((gpio_num_t)YELLOW_GPIO, 0);
    gpio_set_level((gpio_num_t)GREEN_GPIO, 0);

//     lcdClear(&lcd);

//   /* Custom text */
//   char buffer[16] = "E";
//   float version = 1.0f;
//   char initial[2] = {'J', 'M'};
//   //sprintf(buffer, "ESP v%.1f %c%c", version, initial[0], initial[1]);

//   /* Set text */

//   lcd_err_t ret = lcdSetText(&lcd, buffer, 0, 0);

//   /* Check lcd status */
//   assert_lcd(ret);
}

void StatusDisplay::SetRunning()
{
    ESP_LOGI(TAG, "Setting status to running");
    gpio_set_level((gpio_num_t)RED_GPIO, 0);
    gpio_set_level((gpio_num_t)YELLOW_GPIO, 0);
    gpio_set_level((gpio_num_t)GREEN_GPIO, 1);

    // char buffer[15] = "123456789";
     
    // lcd_err_t ret = lcdSetText(buffer, 0, 0);
    // lcdAssert(ret);
}

void StatusDisplay::SetPaused()
{
    ESP_LOGI(TAG, "Setting status to paused");
    gpio_set_level((gpio_num_t)RED_GPIO, 0);
    gpio_set_level((gpio_num_t)YELLOW_GPIO, 1);
    gpio_set_level((gpio_num_t)GREEN_GPIO, 0);

    // char buffer[15] = "PAUSED";
     
    // lcd_err_t ret = lcdSetText(buffer, 0, 0);
    // lcdAssert(ret);
}
