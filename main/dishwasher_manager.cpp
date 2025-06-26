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

TaskHandle_t xHandle = NULL;

esp_err_t DishwasherManager::Init()
{
    ESP_LOGI(TAG, "Initializing DishwasherManager");
    StatusDisplayMgr().Init();
    ModeSelectorMgr().Init();
    UpdateDishwasherDisplay();
    return ESP_OK;
}

uint32_t DishwasherManager::GetTimeRemaining()
{
    return mTimeRemaining;
}

void DishwasherManager::TogglePower()
{
    mIsPoweredOn = !mIsPoweredOn;
    ESP_LOGI(TAG, "Power is %s", mIsPoweredOn ? "on" : "off");

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

void DishwasherManager::TurnOnPower()
{
    mIsPoweredOn = true;
    UpdateDishwasherDisplay();
}

void DishwasherManager::TurnOffPower()
{
    mIsPoweredOn = false;
    UpdateDishwasherDisplay();
}

bool DishwasherManager::IsPoweredOn()
{
    return mIsPoweredOn;
}

static void ProgramTick(void *arg)
{
    // Different Modes can have different times!
    //
    for (;;)
    {
        ESP_LOGI(TAG, "RunProgramTick");
        OperationalStateEnum state = DishwasherMgr().GetOperationalState();

        if (state == OperationalStateEnum::kRunning)
        {
            DishwasherMgr().MoveProgramAlongOneTick();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void DishwasherManager::ToggleProgram()
{
    if (mState == OperationalStateEnum::kStopped)
    {
        mTimeRemaining = 30; // TODO Make this depend on the selected mode.
        xTaskCreate(ProgramTick, "NAME", 4096, NULL, tskIDLE_PRIORITY, &xHandle);
        UpdateOperationState(OperationalStateEnum::kRunning);
    }
    else if (mState == OperationalStateEnum::kRunning)
    {
        vTaskSuspend(xHandle);
    }
}

void DishwasherManager::EndProgram()
{
    if (xHandle != NULL)
    {
        UpdateOperationState(OperationalStateEnum::kStopped);
        vTaskDelete(xHandle);
    }
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

    ESP_LOGI(TAG, "Display: %s", mIsPoweredOn ? "on" : "off");

    if (mIsPoweredOn)
    {
        StatusDisplayMgr().TurnOn();
    }
    else
    {
        StatusDisplayMgr().TurnOff();
    }

    char *mode = "Normal"; // Default value.
    char buffer[64];

    DishwasherModeDelegate *delegate = (DishwasherModeDelegate *)DishwasherMode::GetDelegate();

    if (delegate != nullptr)
    {
        MutableCharSpan label(buffer);

        delegate->GetModeLabelByIndex(mMode, label);

        mode = buffer;
    }

    //ESP_LOGI(TAG, "State: %d", mState);
    //ESP_LOGI(TAG, "Mode: %s", mode);
    //ESP_LOGI(TAG, "TimeRemaining: %lu", mTimeRemaining);

    switch (mState)
    {
    case OperationalStateEnum::kRunning:
        StatusDisplayMgr().UpdateDisplay(RUNNING, mode, mTimeRemaining);
        break;
    case OperationalStateEnum::kPaused:
        StatusDisplayMgr().UpdateDisplay(PAUSED, mode, mTimeRemaining);
        break;
    case OperationalStateEnum::kStopped:
        StatusDisplayMgr().UpdateDisplay(STOPPED, mode, mTimeRemaining);
        break;
    case OperationalStateEnum::kError:
        // sDishwasherLED.Blink(100);
        //  TODO Blink the three LEDS?!
        break;
    default:
        break;
    }
}

void DishwasherManager::MoveProgramAlongOneTick()
{
    mTimeRemaining--;
    if (mTimeRemaining <= 0)
    {
        EndProgram();
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