#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"

#define RED_GPIO 11
#define YELLOW_GPIO 12
#define GREEN_GPIO 13

StatusDisplay StatusDisplay::sStatusDisplay;

esp_err_t StatusDisplay::Init()
{
    gpio_reset_pin((gpio_num_t)RED_GPIO);
    gpio_set_direction((gpio_num_t)RED_GPIO, GPIO_MODE_OUTPUT);

    return ESP_OK;
}

void StatusDisplay::SetRed(bool state)
{
    gpio_set_level((gpio_num_t)RED_GPIO, state ? 1 : 0);
}

void StatusDisplay::SetYellow(bool state)
{
    gpio_set_level((gpio_num_t)YELLOW_GPIO, state ? 1 : 0);
}

void StatusDisplay::SetGreen(bool state)
{
    gpio_set_level((gpio_num_t)GREEN_GPIO, state ? 1 : 0);
}
