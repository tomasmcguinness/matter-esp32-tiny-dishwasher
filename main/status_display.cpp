#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"

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

    return ESP_OK;
}

void StatusDisplay::SetRed(bool state)
{
    ESP_LOGI(TAG, "Setting Red LED to %s", state ? "ON" : "OFF");
    gpio_set_level((gpio_num_t)RED_GPIO, state ? 1 : 0);
}

void StatusDisplay::SetYellow(bool state)
{
    ESP_LOGI(TAG, "Setting Yellow LED to %s", state ? "ON" : "OFF");
    gpio_set_level((gpio_num_t)YELLOW_GPIO, state ? 1 : 0);
}

void StatusDisplay::SetGreen(bool state)
{
     ESP_LOGI(TAG, "Setting Green LED to %s", state ? "ON" : "OFF");
    gpio_set_level((gpio_num_t)GREEN_GPIO, state ? 1 : 0);
}
