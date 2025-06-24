#include <stdio.h>
#include "driver/gpio.h"
#include "lvgl.h"

class StatusDisplay
{
public:
    esp_err_t Init();

    void SetStopped();
    void SetRunning();
    void SetPaused();

private:
    friend StatusDisplay & StatusDisplayMgr(void);
    static StatusDisplay sStatusDisplay;
    lv_disp_t *mDisp;
};

inline StatusDisplay & StatusDisplayMgr(void)
{
    return StatusDisplay::sStatusDisplay;
}