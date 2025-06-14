/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "bsp/esp-bsp.h"
#include <esp_log.h>
#include <app_priv.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::OperationalState;

static const char *TAG = "app_driver";

// chip::Protocols::InteractionModel::Status OperationalStateDelegateImpl::HandleStep(chip::app::Clusters::FanControl::StepDirectionEnum aDirection, bool aWrap, bool aLowestOff)
// {
//     ESP_LOGI(TAG, "Step received value: %d %d %d", (uint8_t)aDirection, aWrap, aLowestOff);

//     return chip::Protocols::InteractionModel::Status::Success;
// }

DataModel::Nullable<uint32_t> OperationalStateDelegateImpl::GetCountdownTime()
{
    return DataModel::MakeNullable((uint32_t)0);
}

CHIP_ERROR OperationalStateDelegateImpl::GetOperationalStateAtIndex(size_t index, GenericOperationalState & operationalState)
{
    ESP_LOGI(TAG, "GetOperationalStateAtIndex");
    return CHIP_NO_ERROR;
}

CHIP_ERROR OperationalStateDelegateImpl::GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase)
{
    ESP_LOGI(TAG, "GetOperationalPhaseAtIndex");
    return CHIP_NO_ERROR;
}

void OperationalStateDelegateImpl::HandlePauseStateCallback(GenericOperationalError & err)
{
    ESP_LOGI(TAG, "HandlePauseStateCallback");
}

void OperationalStateDelegateImpl::HandleResumeStateCallback(GenericOperationalError & err)
{
    ESP_LOGI(TAG, "HandleResumeStateCallback");
}

void OperationalStateDelegateImpl::HandleStartStateCallback(GenericOperationalError & err)
{
     ESP_LOGI(TAG, "HandleStartStateCallback");
}

void OperationalStateDelegateImpl::HandleStopStateCallback(GenericOperationalError & err)
{
     ESP_LOGI(TAG, "HandleStopStateCallback");
}

esp_err_t app_driver_init()
{
    esp_err_t err = ESP_OK;
    return err;
}
