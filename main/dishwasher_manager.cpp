#include "dishwasher_manager.h"

#include "esp_log.h"

#include <app/clusters/operational-state-server/operational-state-server.h>
#include <app/clusters/mode-base-server/mode-base-server.h>

#include "status_display.h"
#include "mode_selector.h"
#include "app_priv.h"

#include <inttypes.h>

static const char *TAG = "dishwasher_manager";

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;

DishwasherManager DishwasherManager::sDishwasher;

static void ProgramTick(void *arg)
{
    while(1)
    {
        ESP_LOGI(TAG, "RunProgramTick");
        DishwasherMgr().ProgressProgram();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

esp_err_t DishwasherManager::Init()
{
    ESP_LOGI(TAG, "Initializing DishwasherManager");
    StatusDisplayMgr().Init();
    ModeSelectorMgr().Init();

    xTaskCreate(ProgramTick, "ProgramTick", 4096, NULL, tskIDLE_PRIORITY, NULL);

    return ESP_OK;
}

uint32_t DishwasherManager::GetTimeRemaining()
{
    return mTimeRemaining;
}

void DishwasherManager::TogglePower()
{
    if (mIsPoweredOn)
    {
        TurnOffPower();
    }
    else
    {
        TurnOnPower();
    }

    uint16_t endpoint_id = 0x01;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    esp_matter::attribute_t *attribute = esp_matter::attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    esp_matter::attribute::get_val(attribute, &val);
    val.val.b = mIsPoweredOn;
    esp_matter::attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

void DishwasherManager::TurnOnPower()
{
    mIsPoweredOn = true;
    StatusDisplayMgr().TurnOn();
}

void DishwasherManager::TurnOffPower()
{
    mIsPoweredOn = false;
    StatusDisplayMgr().TurnOff();
}

bool DishwasherManager::IsPoweredOn()
{
    return mIsPoweredOn;
}

void DishwasherManager::ToggleProgram()
{
    if (mState == OperationalStateEnum::kStopped)
    {
        mTimeRemaining = 30; // TODO Make this depend on the selected mode.
        UpdateOperationState(OperationalStateEnum::kRunning);
    } 
    else if(mState == OperationalStateEnum::kRunning) 
    {
        UpdateOperationState(OperationalStateEnum::kPaused);
    } 
    else if(mState == OperationalStateEnum::kPaused)
    {
        UpdateOperationState(OperationalStateEnum::kRunning);
    }
}

void DishwasherManager::EndProgram()
{
    UpdateOperationState(OperationalStateEnum::kStopped);
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

    char *state_text = "";
    char *mode_text = "";
    char *time_remaining_text = "";

    switch (mState)
    {
    case OperationalStateEnum::kRunning:
        state_text = "RUNNING";
        break;
    case OperationalStateEnum::kPaused:
        state_text = "PAUSED";
        break;
    case OperationalStateEnum::kStopped:
        state_text = "STOPPED";
        break;
    case OperationalStateEnum::kError:
        // sDishwasherLED.Blink(100);
        //  TODO Blink the three LEDS?!
        break;
    default:
        break;
    }

    ESP_LOGI(TAG, "Time Remaining: %lu", mTimeRemaining);

    char time_buffer[30];

    if (mTimeRemaining > 0)
    {
        sprintf(time_buffer, "%lus", mTimeRemaining);
    }

    char buffer[64];

    DishwasherModeDelegate *delegate = (DishwasherModeDelegate *)DishwasherMode::GetDelegate();

    if (delegate != nullptr)
    {
        MutableCharSpan label(buffer);

        delegate->GetModeLabelByIndex(mMode, label);

        mode_text = buffer;
    }

    StatusDisplayMgr().UpdateDisplay(state_text, mode_text, (const char *)&time_buffer);
}

void DishwasherManager::ProgressProgram()
{
    if (mState == OperationalStateEnum::kRunning)
    {
        mTimeRemaining--;

        if (mTimeRemaining <= 0)
        {
            EndProgram();
        }

        UpdateDishwasherDisplay();
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
    DishwasherMgr().UpdateDishwasherDisplay();
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
}