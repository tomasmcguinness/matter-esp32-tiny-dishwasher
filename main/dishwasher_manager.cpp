#include "dishwasher_manager.h"

#include "esp_log.h"

#include <app/clusters/operational-state-server/operational-state-server.h>
#include <app/clusters/mode-base-server/mode-base-server.h>

#include "status_display.h"
#include "mode_selector.h"
#include "app_priv.h"

static const char *TAG = "dishwasher_manager";

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;

DishwasherManager DishwasherManager::sDishwasher;

esp_err_t DishwasherManager::Init()
{
    ESP_LOGI(TAG, "Initializing DishwasherManager");
    StatusDisplayMgr().Init();
    ModeSelectorMgr().Init();
    UpdateDishwasherDisplay();
    return ESP_OK;
}

OperationalStateEnum DishwasherManager::GetOperationalState()
{
    return mState;
}

uint8_t DishwasherManager::GetCurrentMode() 
{
    return mMode;
}

void DishwasherManager::UpdateDishwasherDisplay()
{
    ESP_LOGI(TAG, "UpdateDishwasherDisplay called!");

    OperationalStateEnum opState = DishwasherMgr().GetOperationalState();

    char *mode = "Normal";
    char buffer[64];

    DishwasherModeDelegate *delegate = (DishwasherModeDelegate *)DishwasherMode::GetDelegate();

    if(delegate != nullptr) {
       
        MutableCharSpan label(buffer);

        delegate->GetModeLabelByIndex(mMode, label);

        mode = buffer;
    }

    switch (opState)
    {
    case OperationalStateEnum::kRunning:
        StatusDisplayMgr().UpdateDisplay(RUNNING, mode);
        break;
    case OperationalStateEnum::kPaused:
        StatusDisplayMgr().UpdateDisplay(PAUSED, mode);
        break;
    case OperationalStateEnum::kStopped:
        StatusDisplayMgr().UpdateDisplay(STOPPED, mode);
        break;
    case OperationalStateEnum::kError:
        // sDishwasherLED.Blink(100);
        //  TODO Blink the three LEDS?!
        break;
    default:
        break;
    }
}

void DishwasherManager::UpdateOperationState(OperationalStateEnum state)
{
    mState = state;
    UpdateDishwasherDisplay();
}

void DishwasherManager::UpdateMode(uint8_t mode)
{
    mMode = mode;
    UpdateDishwasherDisplay();
}

static void WorkHandler(intptr_t context)
{
    uint8_t mode = (uint8_t)context;
    DishwasherMode::GetInstance()->UpdateCurrentMode(mode);
}

void DishwasherManager::SelectNextMode()
{
    ESP_LOGI(TAG, "SelectNextMode called!");
    mMode++;

    if(mMode > 2) {
        mMode = 0;
    }

    ESP_LOGI(TAG, "Selected Mode: %d", mMode);
    uint8_t current_mode = DishwasherMode::GetInstance()->GetCurrentMode();

    ESP_LOGI(TAG, "Current Mode: %d", current_mode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(WorkHandler, mMode);

    UpdateDishwasherDisplay();
}

void DishwasherManager::SelectPreviousMode()
{
    uint8_t current_mode = DishwasherMode::GetInstance()->GetCurrentMode();

    ESP_LOGI(TAG, "Current Mode: %d", current_mode);
    
    if(mMode == 0) {
        mMode = 2;
    } else {
        mMode--;
    }

    ESP_LOGI(TAG, "Target Mode: %d", mMode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(WorkHandler, mMode);

    UpdateDishwasherDisplay();
}