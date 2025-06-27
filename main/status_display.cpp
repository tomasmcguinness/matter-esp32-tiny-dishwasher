#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"

#include "driver/i2c_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"

#include "lvgl.h"

#include "dishwasher_manager.h"

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

void update_display_timer(lv_timer_t *timer)
{
    ESP_LOGI(TAG, "update_display_timer tick");

    // uint32_t *user_data = 44;// lv_timer_get_user_data(timer);

    // char buffer[30];
    // sprintf(buffer, "%lus", *user_data);

    // lv_label_set_text(StatusDisplay::mTimeRemainingLabel, "buffer");
    // lv_label_set_text(StatusDisplayMgr().mStateLabel, "STATUS");
    // lv_label_set_text(StatusDisplayMgr().mModeLabel, "MODE");

    // uint32_t time_remaining = DishwasherMgr().GetTimeRemaining();
    // char buffer[30];
    // sprintf(buffer, "%lus", time_remaining);

    // lv_obj_t *label = lv_obj_get_child(lv_scr_act(), 0);
    // lv_label_set_text(label, buffer);

    // label = lv_obj_get_child(lv_scr_act(), 1);
    // lv_label_set_text(label, "Index 1");

    // label = lv_obj_get_child(lv_scr_act(), 2);
    // lv_label_set_text(label, "Index 2");
}

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
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(mPanelHandle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    ESP_LOGI(TAG, "LVGL1");

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

    mDisplayHandle = lvgl_port_add_disp(&disp_cfg);

    ESP_LOGI(TAG, "LVGL2");

    /* Rotation of the screen */
    // lv_disp_set_rotation(mDisplayHandle,  LV_DISP_ROT_NONE);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "DISHWASHER");
    lv_obj_set_width(label, mDisplayHandle->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    mModeLabel = lv_label_create(scr);
    lv_label_set_text(mModeLabel, "");
    lv_obj_set_width(mModeLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mModeLabel, LV_ALIGN_LEFT_MID, 0, 0);

    mStateLabel = lv_label_create(scr);

    lv_label_set_text(mStateLabel, "");
    lv_obj_set_width(mStateLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mStateLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(mStateLabel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mStateLabel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(mStateLabel, lv_color_hex(0xffffff), LV_PART_MAIN);

    mTimeRemainingLabel = lv_label_create(scr);

    lv_label_set_text(mTimeRemainingLabel, "");
    lv_obj_set_width(mTimeRemainingLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mTimeRemainingLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    ESP_LOGI(TAG, "StatusDisplay::Init() finished");

    // static uint32_t user_data = 10;
    // lv_timer_t * timer = lv_timer_create(update_display_timer, 500,  &user_data);

    return ESP_OK;
}

void StatusDisplay::TurnOn()
{
    ESP_LOGI(TAG, "Turning display on");
    esp_lcd_panel_disp_on_off(mPanelHandle, true);
}

void StatusDisplay::TurnOff()
{
    ESP_LOGI(TAG, "Turning display off");
    esp_lcd_panel_disp_on_off(mPanelHandle, false);
}

void StatusDisplay::UpdateDisplay(const char *state_text, const char *mode_text, const char *time_remaining_text)
{
    ESP_LOGI(TAG, "Updating the display");

    lv_label_set_text(mTimeRemainingLabel, time_remaining_text);
    lv_label_set_text(mStateLabel, state_text);
    lv_label_set_text(mModeLabel, mode_text);
}