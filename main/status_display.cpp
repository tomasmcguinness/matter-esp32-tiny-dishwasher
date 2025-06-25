#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"

static const char *TAG = "status_display";

#define I2C_BUS_PORT 0

// TODO Move to configuration
//
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA 22
#define EXAMPLE_PIN_NUM_SCL 23
#define EXAMPLE_PIN_NUM_RST 16
#define EXAMPLE_I2C_HW_ADDR 0x3C

#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LCD_H_RES 128
#define EXAMPLE_LCD_V_RES 64

StatusDisplay StatusDisplay::sStatusDisplay;

// void my_timer(lv_timer_t * timer)
// {
//     ESP_LOGI(TAG, "lv_timer tick");

//     uint32_t *user_data = lv_timer_get_user_data(timer);

//     lv_label_set_text(mStateLabel, "STATUS");
//     lv_label_set_text(mModeLabel, "MODE");
// }

esp_err_t StatusDisplay::Init()
{
    ESP_LOGI(TAG, "StatusDisplay::Init()");

    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = (gpio_num_t)EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = (gpio_num_t)EXAMPLE_PIN_NUM_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        }};
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
        .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    //esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .bits_per_pixel = 1,
    };

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &mPanelHandle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(mPanelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(mPanelHandle));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = mPanelHandle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }};

    mDisp = lvgl_port_add_disp(&disp_cfg);

    /* Rotation of the screen */
    lv_disp_set_rotation(mDisp, LV_DISP_ROT_NONE);

    lv_obj_t *scr = lv_disp_get_scr_act(mDisp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "DISHWASHER");
    lv_obj_set_width(label, mDisp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    mModeLabel = lv_label_create(scr);

    lv_label_set_text(mModeLabel, "");
    lv_obj_set_width(mModeLabel, mDisp->driver->hor_res);
    lv_obj_align(mModeLabel, LV_ALIGN_LEFT_MID, 0, 0);

    mStateLabel = lv_label_create(scr);

    lv_label_set_text(mStateLabel, "");
    lv_obj_set_width(mStateLabel, mDisp->driver->hor_res);
    lv_obj_align(mStateLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(mStateLabel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mStateLabel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(mStateLabel, lv_color_hex(0xffffff), LV_PART_MAIN);

    ESP_LOGI(TAG, "StatusDisplay::Init() finished");

    // static uint32_t user_data = 10;
    // lv_timer_t * timer = lv_timer_create(my_timer, 500,  &user_data);

    return ESP_OK;
}

void StatusDisplay::TurnOn()
{
    ESP_LOGI(TAG, "Turning display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(mPanelHandle, true));
}

void StatusDisplay::TurnOff()
{
    ESP_LOGI(TAG, "Turning display off");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(mPanelHandle, false));
}

void StatusDisplay::UpdateDisplay(State state, const char *mode_text)
{
    ESP_LOGI(TAG, "Setting status");

    char *state_text;

    switch (state)
    {
    case RUNNING:
        state_text = "RUNNING";
        break;
    case STOPPED:
        state_text = "STOPPED";
        break;
    case PAUSED:
        state_text = "PAUSED";
        break;
    default:
        state_text = "ERROR";
        break;
    }

    lv_label_set_text(mStateLabel, state_text);
    lv_label_set_text(mModeLabel, mode_text);
}