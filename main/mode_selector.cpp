#include "mode_selector.h"
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

#include <driver/pulse_cnt.h>
#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "dishwasher_manager.h"

#define EXAMPLE_PCNT_HIGH_LIMIT 100
#define EXAMPLE_PCNT_LOW_LIMIT -100

static const char *TAG = "mode_selector";

static pcnt_unit_handle_t pcnt_unit = NULL;
static QueueHandle_t gpio_pulse_evt_queue = NULL;

static int current_pulse_count = 0;
static int pulse_count = 0;
static int event_count = 0;

ModeSelector ModeSelector::sModeSelector;

#define EXAMPLE_PCNT_HIGH_LIMIT 100
#define EXAMPLE_PCNT_LOW_LIMIT -100

#define CONTACT_SENSOR_GPIO GPIO_NUM_10

static void pulse_counter_monitor_task(void *arg)
{
    while (1)
    {
        if (xQueueReceive(gpio_pulse_evt_queue, &event_count, pdMS_TO_TICKS(1000)))
        {
            ESP_LOGI(TAG, "Watch point event, count: %d", event_count);
        }
        else
        {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            ESP_LOGI(TAG, "Current pulse count: %d", pulse_count);

            if (pulse_count != current_pulse_count)
            {
                int pulse_difference = pulse_count - current_pulse_count;

                current_pulse_count = pulse_count;

                ESP_LOGI(TAG, "Pulse Difference: %d", pulse_difference);

                if(pulse_difference < 0)
                {
                    DishwasherMgr().SelectNextMode();
                }
                else
                {
                    DishwasherMgr().SelectPreviousMode();
                }
            }
        }
    }
}

esp_err_t ModeSelector::Init()
{
    ESP_LOGI(TAG, "install pcnt unit");

    pcnt_unit_config_t unit_config = {
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
    };

    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG, "set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = GPIO_NUM_2,
        .level_gpio_num = GPIO_NUM_11,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = GPIO_NUM_11,
        .level_gpio_num = GPIO_NUM_2,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "add watch points and register callbacks");
    int watch_points[] = {EXAMPLE_PCNT_LOW_LIMIT, -50, 0, 50, EXAMPLE_PCNT_HIGH_LIMIT};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++)
    {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }
    pcnt_event_callbacks_t cbs = {
        //.on_reach = example_pcnt_on_reach,
    };

    gpio_pulse_evt_queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, gpio_pulse_evt_queue));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // Listen for the click.
    //
    // button_config_t config;
    // memset(&config, 0, sizeof(button_config_t));

    // config.type = BUTTON_TYPE_GPIO;
    // config.gpio_button_config.gpio_num = GPIO_NUM_10;
    // config.gpio_button_config.active_level = 0;

    // To enable powersafe, a setting must be set first via menuconfig
    // config.gpio_button_config.enable_power_save = true;

    //button_handle_t handle = iot_button_create(&config);

    //ESP_ERROR_CHECK(iot_button_register_cb(handle, BUTTON_PRESS_DOWN, app_driver_button_single_click_cb, NULL));

    xTaskCreate(pulse_counter_monitor_task, "pulse_counter_monitor_task", 4096, NULL, 10, NULL);

    //client::set_request_callback(app_driver_client_invoke_command_callback, app_driver_client_group_invoke_command_callback, NULL);

    return ESP_OK;
}