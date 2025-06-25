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

void DishwasherManager::TogglePower()
{
    ESP_LOGI(TAG,"Power is %s", mIsPoweredOn ? "on" : "off");
    mIsPoweredOn = !mIsPoweredOn;
    ESP_LOGI(TAG,"Power is %s", mIsPoweredOn ? "on" : "off");

    uint16_t endpoint_id = 0x01;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    esp_matter::attribute_t *attribute = esp_matter::attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    esp_matter::attribute::get_val(attribute, &val);
    val.val.b = mIsPoweredOn;
    esp_matter::attribute::update(endpoint_id, cluster_id, attribute_id, &val);

    UpdateDishwasherDisplay();
}

bool DishwasherManager::IsPoweredOn()
{
    return mIsPoweredOn;
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

    if (mIsPoweredOn)
    {
        StatusDisplayMgr().TurnOn();
    }
    else
    {
        StatusDisplayMgr().TurnOff();
    }

    OperationalStateEnum opState = DishwasherMgr().GetOperationalState();

    char *mode = "Normal";
    char buffer[64];

    DishwasherModeDelegate *delegate = (DishwasherModeDelegate *)DishwasherMode::GetDelegate();

    if (delegate != nullptr)
    {
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

    // Roll over if we reach the end
    //
    if (mMode > 2)
    {
        mMode = 0;
    }

    ESP_LOGI(TAG, "Selected Mode: %d", mMode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(WorkHandler, mMode);

    UpdateDishwasherDisplay();
}

void DishwasherManager::SelectPreviousMode()
{
    uint8_t current_mode = DishwasherMode::GetInstance()->GetCurrentMode();

    ESP_LOGI(TAG, "Current Mode: %d", current_mode);

    // Roll over if we reach the start
    //
    if (mMode == 0)
    {
        mMode = 2;
    }
    else
    {
        mMode--;
    }

    ESP_LOGI(TAG, "Target Mode: %d", mMode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(WorkHandler, mMode);

    UpdateDishwasherDisplay();
}