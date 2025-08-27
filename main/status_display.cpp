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

    lv_disp_set_rotation(mDisplayHandle, LV_DISP_ROT_180);

    ESP_LOGI(TAG, "LVGL2");

    lv_obj_t *scr = lv_scr_act();

    mModeLabel = lv_label_create(scr);
    lv_label_set_text(mModeLabel, "");
    lv_obj_set_width(mModeLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mModeLabel, LV_ALIGN_LEFT_MID, 0, 0);

    mStateLabel = lv_label_create(scr);

    lv_label_set_text(mStateLabel, "");
    lv_obj_set_width(mStateLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mStateLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(mStateLabel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mStateLabel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(mStateLabel, lv_color_hex(0xffffff), LV_PART_MAIN);

    mStatusLabel = lv_label_create(scr);

    lv_label_set_text(mStatusLabel, "");
    lv_obj_set_width(mStatusLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mStatusLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    mResetMessageLabel = lv_label_create(scr);

    lv_label_set_text(mResetMessageLabel, "Reset the device?");
    lv_obj_set_width(mResetMessageLabel, mDisplayHandle->driver->hor_res);
    lv_obj_add_flag(mResetMessageLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(mResetMessageLabel, LV_ALIGN_TOP_MID, 0, 0);

    mYesButtonLabel = lv_label_create(scr);

    lv_label_set_text(mYesButtonLabel, "Yes");
    lv_obj_set_width(mYesButtonLabel, mDisplayHandle->driver->hor_res);
    lv_obj_add_flag(mYesButtonLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(mYesButtonLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(mYesButtonLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    mNoButtonLabel = lv_label_create(scr);

    lv_label_set_text(mNoButtonLabel, "No");
    lv_obj_set_width(mNoButtonLabel, mDisplayHandle->driver->hor_res);
    lv_obj_add_flag(mNoButtonLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(mNoButtonLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(mNoButtonLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    mStartsInLabel = lv_label_create(scr);

    lv_label_set_text(mStartsInLabel, "");
    lv_obj_set_width(mStartsInLabel, mDisplayHandle->driver->hor_res);
    lv_obj_add_flag(mStartsInLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(mStartsInLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(mStartsInLabel, LV_ALIGN_CENTER, 0, 0);

    ESP_LOGI(TAG, "StatusDisplay::Init() finished");

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

void StatusDisplay::UpdateDisplay(int32_t startsIn , const char *state_text, const char *mode_text, const char *status_text)
{
    ESP_LOGI(TAG, "Updating the display: There is a %lus delay.", startsIn);

    if (startsIn == 0)
    {
        ESP_LOGI(TAG, "state_text: [%s]", state_text);
        ESP_LOGI(TAG, "mode_text: [%s]", mode_text);
        ESP_LOGI(TAG, "status_text: [%s]", status_text);

        lv_obj_clear_flag(mStateLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(mModeLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(mStatusLabel, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_flag(mStartsInLabel, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(mStateLabel, state_text);
        lv_label_set_text(mModeLabel, mode_text);
        lv_label_set_text(mStatusLabel, status_text);
    }
    else
    {
        lv_obj_add_flag(mStateLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mModeLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mStatusLabel, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(mStartsInLabel, LV_OBJ_FLAG_HIDDEN);

        char *starts_in_formatted_buffer = (char *)malloc(128);
        snprintf(starts_in_formatted_buffer, 128, "Starting in %lus", startsIn);

        lv_label_set_text(mStartsInLabel, starts_in_formatted_buffer);
    }
}

void StatusDisplay::ShowResetOptions()
{
    ESP_LOGI(TAG, "Show reset options");

    lv_obj_add_flag(mStateLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mModeLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mStatusLabel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(mResetMessageLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mYesButtonLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mNoButtonLabel, LV_OBJ_FLAG_HIDDEN);
}

void StatusDisplay::HideResetOptions()
{

    ESP_LOGI(TAG, "Hide reset options");

    lv_obj_clear_flag(mStateLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mModeLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mStatusLabel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(mResetMessageLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mYesButtonLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mNoButtonLabel, LV_OBJ_FLAG_HIDDEN);
}