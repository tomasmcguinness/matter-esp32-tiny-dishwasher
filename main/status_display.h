#include <stdio.h>
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include <inttypes.h>

enum State {
  STOPPED,
  RUNNING,
  PAUSED
}; 

class StatusDisplay
{
public:
    esp_err_t Init();

    void TurnOn();
    void TurnOff();

    void UpdateDisplay(int32_t startsIn, const char *state_text, const char *mode_text, const char *status_text);

    void ShowResetOptions();
    void HideResetOptions();

private:
    friend StatusDisplay & StatusDisplayMgr(void);
    static StatusDisplay sStatusDisplay;
    lv_disp_t *mDisplayHandle;
    esp_lcd_panel_handle_t mPanelHandle;

    lv_obj_t *mStatusLabel;
    lv_obj_t *mStateLabel;
    lv_obj_t *mModeLabel;
    lv_obj_t *mResetMessageLabel;
    lv_obj_t *mYesButtonLabel;
    lv_obj_t *mNoButtonLabel;
    lv_obj_t *mStartsInLabel;
};

inline StatusDisplay & StatusDisplayMgr(void)
{
    return StatusDisplay::sStatusDisplay;
}