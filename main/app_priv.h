/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app/clusters/operational-state-server/operational-state-server.h>
#include <app/clusters/fan-control-server/fan-control-delegate.h>
#include <protocols/interaction_model/StatusCode.h>
typedef void *app_driver_handle_t;

esp_err_t app_driver_init();

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif

class OperationalStateDelegateImpl:public chip::app::Clusters::FanControl::Delegate
{
public:
    OperationalStateDelegateImpl(uint16_t aEndpoint):Delegate(aEndpoint){}

    chip::Protocols::InteractionModel::Status HandleStep(chip::app::Clusters::FanControl::StepDirectionEnum aDirection, bool aWrap, bool aLowestOff);
};