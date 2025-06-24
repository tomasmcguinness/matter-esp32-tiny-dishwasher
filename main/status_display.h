#include <stdio.h>
#include "driver/gpio.h"
#include "lvgl.h"

enum State {
  STOPPED,
  RUNNING,
  PAUSED
}; 

class StatusDisplay
{
public:
    esp_err_t Init();
    void UpdateDisplay(State state, const char* mode);

private:
    friend StatusDisplay & StatusDisplayMgr(void);
    static StatusDisplay sStatusDisplay;
    lv_disp_t *mDisp;
    lv_obj_t *mModeLabel;
    lv_obj_t *mStateLabel;
};

inline StatusDisplay & StatusDisplayMgr(void)
{
    return StatusDisplay::sStatusDisplay;
}