/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "bsp/esp-bsp.h"

#include <app_priv.h>

static const char *TAG = "app_driver";

chip::Protocols::InteractionModel::Status OperationalStateDelegateImpl::HandleStep(chip::app::Clusters::FanControl::StepDirectionEnum aDirection, bool aWrap, bool aLowestOff)
{
    ESP_LOGI(TAG, "Step received value: %d %d %d", (uint8_t)aDirection, aWrap, aLowestOff);

    return chip::Protocols::InteractionModel::Status::Success;
}

esp_err_t app_driver_init()
{
    esp_err_t err = ESP_OK;
    return err;
}
