#include "dishwasher_manager.h"

#include "esp_log.h"

#include <app/clusters/operational-state-server/operational-state-server.h>

#include "status_display.h"

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
    return ESP_OK;
}

OperationalStateEnum DishwasherManager::GetOperationalState()
{
    return mState;
}

void DishwasherManager::UpdateDishwasherLed()
{
    ESP_LOGI(TAG, "UpdateDishwasherLed called!");

    OperationalStateEnum opState = DishwasherMgr().GetOperationalState();

    switch (opState)
    {
    case OperationalStateEnum::kRunning:
        StatusDisplayMgr().SetRed(false);
        StatusDisplayMgr().SetYellow(false);
        StatusDisplayMgr().SetGreen(true);
        break;
    case OperationalStateEnum::kPaused:
        StatusDisplayMgr().SetRed(false);
        StatusDisplayMgr().SetYellow(true);
        StatusDisplayMgr().SetGreen(false);
        break;
    case OperationalStateEnum::kStopped:
        StatusDisplayMgr().SetRed(true);
        StatusDisplayMgr().SetYellow(false);
        StatusDisplayMgr().SetGreen(false);
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
    UpdateDishwasherLed();
}