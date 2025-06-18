/**
 *
 * @file lcd1602_driver.c
 * @author Aad van Gerwen
 * @brief Driver for LCD 1602
 * @version 0.2
 * @date 2025-03-17
 *
 * @copyright Copyright (c) 2025
 *
 **/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "1602_lcd.h"


#define LCD_DATA 0          // LCD data
#define LCD_CMD 1           // LCD command
#define GPIO_STATE_LOW 0    // Logic low
#define GPIO_STATE_HIGH 1   // Logic high


#define LCD_DATA_LINE 4     //  4-Bit data line
// Default pinout  in 4- bits mode
#define DATA_0_PIN          CONFIG_DATA_0_PIN          // DATA 4 via sdkconfig
#define DATA_1_PIN          CONFIG_DATA_1_PIN          // DATA 5
#define DATA_2_PIN          CONFIG_DATA_2_PIN          // DATA 6
#define DATA_3_PIN          CONFIG_DATA_3_PIN          // DATA 7
#define ENABLE_PIN          CONFIG_ENABLE_PIN          // Enable
#define REGISTER_SELECT_PIN CONFIG_REGISTER_SELECT_PIN //  Register Select23


/******************************************************************
 * \brief LCD state enumeration
 *******************************************************************/
typedef enum {
    LCD_INACTIVE = 0,   /* LCD inactive */
    LCD_ACTIVE = 1,     /* LCD active   */
} lcd_state_t;


/******************************************************************
 * \struct lcd_t
 * \brief LCD object
 *******************************************************************/
typedef struct {
    gpio_num_t data[LCD_DATA_LINE]; /* GPIO Pin numbers of the LCD data lines  */
    lcd_state_t state;              /* LCD state  */
} lcd_t;


// Create and initialize LCD object
static lcd_t s_lcd = {{DATA_0_PIN, DATA_1_PIN, DATA_2_PIN, DATA_3_PIN}, LCD_INACTIVE};

// LCD Logging tag
static const char *s_lcd_tag = "LCD tag";


/*
 * @brief Trigger LCD enable pin data is written to the LCD on falling edge
 *
 * @param  None
 * @return None
 *
 */
static void lcdTriggerEN(void)
{
    gpio_set_level(ENABLE_PIN, GPIO_STATE_LOW);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(ENABLE_PIN, GPIO_STATE_HIGH);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(ENABLE_PIN, GPIO_STATE_LOW);
    vTaskDelay(1 / portTICK_PERIOD_MS);
}


/**
 * @brief Transmitt lower nibble of the 8 bits char
 *
 * @param x    b3 .. b0 of char to be transmiter
 * @return None
 */
static void lownibble(unsigned char x)
{
    uint8_t i, val = 0x01;
    for (i = 0; i < LCD_DATA_LINE; i++) {
        // check if x is high for every bit
        gpio_set_level(s_lcd.data[i], (x >> i) & val);
    }
}


/**
 * @brief Write command to LCD object
 *
 * @param cmd       LCD command
 * @param lcd_opt   0: data , 1: command
 * @return None
 */
static void lcdWriteCmd(unsigned char cmd, uint8_t lcd_opt)
{
    // CMD: 1, DATA: 0
    if (lcd_opt == LCD_CMD) {
        gpio_set_level(REGISTER_SELECT_PIN, GPIO_STATE_LOW);
    } else {
        gpio_set_level(REGISTER_SELECT_PIN, GPIO_STATE_HIGH);
    }

    // upper 4 bits
    lownibble(cmd >> 4);
    lcdTriggerEN();

    // lower 4 bits
    lownibble(cmd);
    lcdTriggerEN();

    // CMD : 1, DATA: 0
    if (lcd_opt == LCD_CMD) {
        vTaskDelay(10 / portTICK_PERIOD_MS); /* 10 ms delay */
    }
}


/**
 * @brief Initialize LCD object
 *
 * @param lcd   pointer to LCD object
 * @note  Must constructor LCD object. @see lcd_ctor() and @see lcd_default()
 * @return None
 */
void lcdInit(void)
{
    int i;

    // Select enable (EN) and register select (RS) pin
    esp_rom_gpio_pad_select_gpio(ENABLE_PIN);
    esp_rom_gpio_pad_select_gpio(REGISTER_SELECT_PIN);

    // Set en and register select pin as output
    gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(REGISTER_SELECT_PIN, GPIO_MODE_OUTPUT);

    // Set en and register select pin as low
    gpio_set_level(ENABLE_PIN, GPIO_STATE_LOW);
    gpio_set_level(REGISTER_SELECT_PIN, GPIO_STATE_LOW);

    // Select all data pins
    for (i = 0; i < LCD_DATA_LINE; i++) {
        esp_rom_gpio_pad_select_gpio(s_lcd.data[i]);
    }

    // Set all data pins as output
    for (i = 0; i < LCD_DATA_LINE; i++) {
        gpio_set_direction(s_lcd.data[i], GPIO_MODE_OUTPUT);
    }

    // Set all data pins output as low
    for (i = 0; i < LCD_DATA_LINE; i++) {
        gpio_set_level(s_lcd.data[i], GPIO_STATE_LOW);
    }

    s_lcd.state = LCD_ACTIVE;

    // 100 ms delay
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // set 0x03 to LCD
    gpio_set_level(ENABLE_PIN, GPIO_STATE_LOW);
    gpio_set_level(REGISTER_SELECT_PIN, GPIO_STATE_LOW);
    gpio_set_level(DATA_0_PIN, GPIO_STATE_HIGH);
    gpio_set_level(DATA_1_PIN, GPIO_STATE_HIGH);
    gpio_set_level(DATA_2_PIN, GPIO_STATE_LOW);
    gpio_set_level(DATA_3_PIN, GPIO_STATE_LOW);

    // Send 0x03 3 times at 10ms then 100 us
    lcdTriggerEN();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcdTriggerEN();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcdTriggerEN();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcdTriggerEN();

    // switch to 4-bit mode, 0x02
    gpio_set_level(ENABLE_PIN, GPIO_STATE_LOW);
    gpio_set_level(REGISTER_SELECT_PIN, GPIO_STATE_LOW);
    gpio_set_level(DATA_0_PIN, GPIO_STATE_LOW);
    gpio_set_level(DATA_1_PIN, GPIO_STATE_HIGH);
    gpio_set_level(DATA_2_PIN, GPIO_STATE_LOW);
    gpio_set_level(DATA_3_PIN, GPIO_STATE_LOW);

    // Trigger enable
    lcdTriggerEN();
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Initialize LCD
    lcdWriteCmd(0x28, LCD_CMD); // 4-bit: DL=0, 2-line: N=1, 5x8: F=0
    lcdWriteCmd(0x08, LCD_CMD); // Instruction Flow was Display Off: D=0 cursor off C=0 blinking off B=0
    lcdWriteCmd(0x01, LCD_CMD); // Clear LCD cursor home
    lcdWriteCmd(0x06, LCD_CMD); // Auto-Increment I/D=1 S=0
    lcdWriteCmd(0x0C, LCD_CMD); // Display On, No blink was 0c
}


/**
 * @brief Set text
 *
 * Detailed description starts here
 * @param text  string text
 * @param x     location at x-axis
 * @param y     location at y-axis
 * @return      lcd error status @see lcd_err_t
 */
lcd_err_t lcdSetText(char *text, int x, int y)
{
    // Check if lcd is active
    if (s_lcd.state == LCD_ACTIVE) {
        int i;
        if (x < 16) {
            x |= 0x80; // Set LCD for first line write
            switch (y) {
            case 1:
                x |= 0x40; // Set LCD for second line write
                break;
            case 2:
                x |= 0x60; // Set LCD for first line write reverse
                break;
            case 3:
                x |= 0x20; // Set LCD for second line write reverse
                break;
            }
            lcdWriteCmd(x, LCD_CMD);
        }
        i = 0;
        // Write text
        while (text[i] != '\0') {
            lcdWriteCmd(text[i], LCD_DATA);
            i++;
        }
    }
    // return lcd status
    return s_lcd.state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
}


/**
 * @brief Set integer
 *
 * Detailed description starts here
 * @param val   integer value to be displayed
 * @param x     location at x-axis
 * @param y     location at y-axis
 * @return      lcd error status @see lcd_err_t
 */
lcd_err_t lcdSetInt(int val, int x, int y)
{
    // Check if lcd is active
    if (s_lcd.state == LCD_ACTIVE) {
        // Store integer to buffer
        char buffer[16];
        sprintf(buffer, "%d", val);
        // Set integer
        lcdSetText(buffer, x, y);
    }
    // return lcd status
    return s_lcd.state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
}


/**
 * @brief Clear LCD screen
 * Detailed description starts here
 * @return      lcd error status @see lcd_err_t
 */
lcd_err_t lcdClear(void)
{
    // Check if lcd is active
    if (s_lcd.state == LCD_ACTIVE) {
        // Clear LCD screen
        lcdWriteCmd(0x01, LCD_CMD);
    }

    // return lcd status
    return s_lcd.state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
}


void lcdAssert(lcd_err_t lcd_error)
{
    if (lcd_error != LCD_OK) {
        ESP_LOGE(s_lcd_tag, "LCD has failed!!! 0x%x\n", lcd_error); // Display error message
    }
}