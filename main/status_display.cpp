#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"
#include "esp_lcd.h"

static const char *TAG = "status_display";

StatusDisplay StatusDisplay::sStatusDisplay;

static lcd_t lcd;

esp_err_t StatusDisplay::Init()
{
    ESP_LOGI(TAG, "StatusDisplay::Init()");

    lcdDefault(&lcd);

    lcdInit(&lcd);

    /* Clear previous data on LCD */
    lcd_err_t ret = lcdClear(&lcd);

    assert_lcd(ret);

     /* Custom text */
    char buffer[16] = "E";
    float version = 1.0f;
    char initial[2] = {'J', 'M'};
    sprintf(buffer, "ESP v%.1f %c%c", version, initial[0], initial[1]);

    /* Set text */
    ret = lcdSetText(&lcd, buffer, 0, 0);

    /* Check lcd status */
    assert_lcd(ret);

    ESP_LOGI(TAG, "StatusDisplay::Init() finished");

    return ESP_OK;
}

void StatusDisplay::SetStopped()
{
    //lcd_err_t ret = lcdClear();
    //lcdAssert(ret);

    ESP_LOGI(TAG, "Setting status to stopped");

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
    char buffer[15] = "RUNNING";
    lcd_err_t ret = lcdSetText(&lcd, buffer, 0, 0);
    assert_lcd(ret);
}

void StatusDisplay::SetPaused()
{
    ESP_LOGI(TAG, "Setting status to paused");

    // char buffer[15] = "PAUSED";
     
    // lcd_err_t ret = lcdSetText(buffer, 0, 0);
    // lcdAssert(ret);
}