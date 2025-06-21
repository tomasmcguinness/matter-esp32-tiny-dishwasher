#include <stdio.h>
#include <esp_err.h>

class ModeSelector
{
public:
    esp_err_t Init();

private:
    friend ModeSelector & ModeSelectorMgr(void);
    static ModeSelector sModeSelector;
};

inline ModeSelector & ModeSelectorMgr(void)
{
    return ModeSelector::sModeSelector;
}