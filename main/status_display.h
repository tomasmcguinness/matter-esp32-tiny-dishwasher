#include <stdio.h>
#include "driver/gpio.h"

class StatusDisplay
{
public:
    esp_err_t Init();

    void SetRed(bool state);
    void SetYellow(bool state);
    void SetGreen(bool state);

private:
    friend StatusDisplay & StatusDisplayMgr(void);
    static StatusDisplay sStatusDisplay;
};

inline StatusDisplay & StatusDisplayMgr(void)
{
    return StatusDisplay::sStatusDisplay;
}