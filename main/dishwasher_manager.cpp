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
    while (1)
    {
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

    // We can update the OnOff attribute directly as its managed by esp-matter.
    //
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
    UpdateDishwasherDisplay();
}

void DishwasherManager::TurnOffPower()
{
    mIsPoweredOn = false;
    StopProgram();
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
        StartProgram();
    }
    else if (mState == OperationalStateEnum::kRunning)
    {
        PauseProgram();
    }
    else if (mState == OperationalStateEnum::kPaused)
    {
        ResumeProgram();
    }
}

// TODO Return an error!
void DishwasherManager::StartProgram()
{
    mTimeRemaining = 30; // TODO Make this depend on the selected mode.
    mPhase = 0;
    UpdateCurrentPhase(mPhase);
    UpdateOperationState(OperationalStateEnum::kRunning);

    SetForecast();
}

void DishwasherManager::PauseProgram()
{
    UpdateOperationState(OperationalStateEnum::kPaused);
}

void DishwasherManager::ResumeProgram()
{
    UpdateOperationState(OperationalStateEnum::kRunning);
}

void DishwasherManager::StopProgram()
{
    mTimeRemaining = 0;
    UpdateCurrentPhase(0);
    UpdateMode(0);
    UpdateOperationState(OperationalStateEnum::kStopped);
}

void DishwasherManager::EndProgram()
{
    // TODO We might want to do other stuff here, like raise a Matter event that the program has ended.
    //
    StopProgram();
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
    char *status_text = "";

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

    char time_buffer[30] = "";

    if (mTimeRemaining > 0)
    {
        sprintf(time_buffer, "%lus", mTimeRemaining);
    }

    char mode_buffer[64];

    DishwasherModeDelegate *delegate = (DishwasherModeDelegate *)DishwasherMode::GetDelegate();

    if (delegate != nullptr)
    {
        MutableCharSpan label(mode_buffer);

        delegate->GetModeLabelByIndex(mMode, label);

        mode_text = mode_buffer;
    }

    char status_buffer[64];
    char *status_formatted_buffer = NULL;

    if (mState == OperationalStateEnum::kRunning || mState == OperationalStateEnum::kPaused)
    {
        OperationalStateDelegate *operational_state_delegate = (OperationalStateDelegate *)OperationalState::GetDelegate();

        if (operational_state_delegate != nullptr)
        {
            MutableCharSpan label(status_buffer);

            operational_state_delegate->GetOperationalPhaseAtIndex(mPhase, label);

            int length = snprintf((char *)NULL, 0, "%s (%s)", time_buffer, status_buffer) + 1; /* +1 for the null terminator */
            status_formatted_buffer = (char *)malloc(length);
            snprintf(status_formatted_buffer, length, "%s (%s)", time_buffer, status_buffer);

            status_text = status_formatted_buffer;
        }
    }

    StatusDisplayMgr().UpdateDisplay(state_text, mode_text, status_text);

    if (status_formatted_buffer != NULL)
    {
        free(status_formatted_buffer);
    }
}

void DishwasherManager::ProgressProgram()
{
    if (mState == OperationalStateEnum::kRunning)
    {
        mTimeRemaining--;

        uint8_t current_phase = 0;

        if (mTimeRemaining <= 0)
        {
            current_phase = 0;
            EndProgram();
        }
        else if (mTimeRemaining < 5)
        {
            current_phase = 4;
        }
        else if (mTimeRemaining < 10)
        {
            current_phase = 3;
        }
        else if (mTimeRemaining < 15)
        {
            current_phase = 2;
        }
        else if (mTimeRemaining < 20)
        {
            current_phase = 1;
        }

        UpdateCurrentPhase(current_phase);
    }
}

static void UpdateOperationalStatePhaseWorkHandler(intptr_t context)
{
    ESP_LOGI(TAG, "UpdateOperationalStatePhaseWorkHandler()");
    DataModel::Nullable<uint8_t> phase = (DataModel::Nullable<uint8_t>)context;
    OperationalState::GetInstance()->SetCurrentPhase(phase);
    DishwasherMgr().UpdateDishwasherDisplay();
}

void DishwasherManager::UpdateCurrentPhase(uint8_t phase)
{
    mPhase = phase;

    // This is one way to perform safe changes to the Matter stack.
    //
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateOperationalStatePhaseWorkHandler, mPhase);

    // This is another.
    //
    // chip::DeviceLayer::PlatformMgr().LockChipStack();
    // chip::DeviceLayer::PlatformMgr().UnlockChipStack();
}

static void UpdateOperationalStateWorkHandler(intptr_t context)
{
    ESP_LOGI(TAG, "UpdateOperationalStateWorkHandler()");
    OperationalState::OperationalStateEnum state = (OperationalState::OperationalStateEnum)context;
    OperationalState::GetInstance()->SetOperationalState(to_underlying(state));
    OperationalState::GetInstance()->UpdateCountdownTimeFromDelegate();
    DishwasherMgr().UpdateDishwasherDisplay();
}

void DishwasherManager::UpdateOperationState(OperationalStateEnum state)
{
    mState = state;
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateOperationalStateWorkHandler, (uint8_t)mState);
}

void DishwasherManager::UpdateMode(uint8_t mode)
{
    mMode = mode;
    UpdateDishwasherDisplay();
}

static void UpdateDishwasherCurrentModeWorkHandler(intptr_t context)
{
    ESP_LOGI(TAG, "UpdateOperationalStatePhaseWorkHandler()");
    uint8_t mode = (uint8_t)context;
    DishwasherMode::GetInstance()->UpdateCurrentMode(mode);
    DishwasherMgr().UpdateDishwasherDisplay();
}

void DishwasherManager::SelectNextMode()
{
    ESP_LOGI(TAG, "SelectNextMode called!");

    if (mState != OperationalStateEnum::kStopped)
    {
        ESP_LOGI(TAG, "Mode can only be changed when dishwasher is stopped!");
        return;
    }

    mMode++;

    // Roll over if we reach the end
    //
    if (mMode > 2)
    {
        mMode = 0;
    }

    ESP_LOGI(TAG, "Selected Mode: %d", mMode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateDishwasherCurrentModeWorkHandler, mMode);
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

    ESP_LOGI(TAG, "Selected Mode: %d", mMode);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateDishwasherCurrentModeWorkHandler, mMode);
}

// static void UpdateForecastWorkHandler(intptr_t context)
// {
//     ESP_LOGI(TAG, "UpdateOperationalStatePhaseWorkHandler()");
//     uint8_t mode = (uint8_t)context;
//     DishwasherMode::GetInstance()->UpdateCurrentMode(mode);
//     DishwasherMgr().UpdateDishwasherDisplay();
// }

void DishwasherManager::SetForecast()
{
    uint32_t freeBefore = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG,"Free Before: %lu", freeBefore);

    uint32_t matterEpoch = 1753335026;

    chip::app::Clusters::DeviceEnergyManagement::Structs::ForecastStruct::Type newForecast;

    newForecast.forecastID = 0;
    newForecast.startTime = matterEpoch + 60;
    newForecast.earliestStartTime = MakeOptional(DataModel::MakeNullable(matterEpoch));
    newForecast.endTime = matterEpoch + 60 + mTimeRemaining;
    newForecast.isPausable = true;

    newForecast.activeSlotNumber.SetNonNull(0);

    chip::app::Clusters::DeviceEnergyManagement::Structs::SlotStruct::Type slots[1];

    slots[0].minDuration = 10;
    slots[0].maxDuration = 20;
    slots[0].defaultDuration = 15;
    slots[0].elapsedSlotTime = 0;
    slots[0].remainingSlotTime = 0;

    slots[0].slotIsPausable.SetValue(true);
    slots[0].minPauseDuration.SetValue(10);
    slots[0].maxPauseDuration.SetValue(60);

    slots[0].nominalPower.SetValue(2500000);
    slots[0].minPower.SetValue(1200000);
    slots[0].maxPower.SetValue(7600000);
    slots[0].nominalEnergy.SetValue(2000);

    newForecast.slots = DataModel::List<const DeviceEnergyManagement::Structs::SlotStruct::Type>(slots, 1);

    chip::DeviceLayer::PlatformMgr().LockChipStack();
    device_energy_management_delegate.SetForecast(DataModel::MakeNullable(newForecast));
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    uint32_t freeAfter = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG,"Free: %lu", freeAfter);
}