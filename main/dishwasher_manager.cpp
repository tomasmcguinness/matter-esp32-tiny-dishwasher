#include "dishwasher_manager.h"

#include <app/clusters/operational-state-server/operational-state-server.h>

#include "status_display.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;

DishwasherManager DishwasherManager::sDishwasher;

esp_err_t DishwasherManager::Init()
{
    StatusDisplayMgr().Init();
    return ESP_OK;
}

OperationalStateEnum DishwasherManager::GetOperationalState()
{
    return mState;
}

void DishwasherManager::UpdateDishwasherLed()
{   
    OperationalStateEnum opState = DishwasherMgr().GetOperationalState();

    switch(opState)
    {
        case OperationalStateEnum::kRunning:
            StatusDisplayMgr().SetRed(false);
            StatusDisplayMgr().SetYellow(false);
            StatusDisplayMgr().SetGreen(true);
        break;
        case OperationalStateEnum::kPaused:
            //set_red(false);
            //set_yellow(true);
            //set_green(false);
        break;
        case OperationalStateEnum::kStopped:
            //set_red(true);
            //set_yellow(false);
            //set_green(false);
        break;
        case OperationalStateEnum::kError :
            //sDishwasherLED.Blink(100);
            // TODO Blink the three LEDS?!
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