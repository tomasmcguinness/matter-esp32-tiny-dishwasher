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
#include <app-common/zap-generated/ids/Attributes.h>
#include <app/clusters/operational-state-server/operational-state-server.h>
#include <protocols/interaction_model/StatusCode.h>

typedef void *app_driver_handle_t;

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::OperationalState;

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

namespace chip {
namespace app {
namespace Clusters {
namespace OperationalState {

class GenericOperationalStateDelegateImpl : public Delegate
{
public:

    //GenericOperationalStateDelegateImpl():Delegate(){}

    uint32_t mRunningTime = 0;
    uint32_t mPausedTime  = 0;
    app::DataModel::Nullable<uint32_t> mCountDownTime;

    chip::app::DataModel::Nullable<uint32_t> GetCountdownTime() override;

    CHIP_ERROR GetOperationalStateAtIndex(size_t index, GenericOperationalState & operationalState) override;

    CHIP_ERROR GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase) override;

    void HandlePauseStateCallback(GenericOperationalError & err) override;

    void HandleResumeStateCallback(GenericOperationalError & err) override;

    void HandleStartStateCallback(GenericOperationalError & err) override;

    void HandleStopStateCallback(GenericOperationalError & err) override;

protected:
    Span<const GenericOperationalState> mOperationalStateList;
    Span<const CharSpan> mOperationalPhaseList;
};

class OperationalStateDelegate : public GenericOperationalStateDelegateImpl
{
private:
    const GenericOperationalState opStateList[4] = {
        GenericOperationalState(to_underlying(OperationalStateEnum::kStopped)),
        GenericOperationalState(to_underlying(OperationalStateEnum::kRunning)),
        GenericOperationalState(to_underlying(OperationalStateEnum::kPaused)),
        GenericOperationalState(to_underlying(OperationalStateEnum::kError)),
    };

    const uint32_t kExampleCountDown = 30;

public:
    OperationalStateDelegate()
    {
        GenericOperationalStateDelegateImpl::mOperationalStateList = Span<const GenericOperationalState>(opStateList);
    }

    /**
     * Handle Command Callback in application: Start
     * @param[out] get operational error after callback.
     */
    void HandleStartStateCallback(GenericOperationalError & err) override
    {
        mCountDownTime.SetNonNull(static_cast<uint32_t>(kExampleCountDown));
        GenericOperationalStateDelegateImpl::HandleStartStateCallback(err);
    }

    /**
     * Handle Command Callback in application: Stop
     * @param[out] get operational error after callback.
     */
    void HandleStopStateCallback(GenericOperationalError & err) override
    {
        GenericOperationalStateDelegateImpl::HandleStopStateCallback(err);
        mCountDownTime.SetNull();
    }
};

Instance * GetOperationalStateInstance();
OperationalStateDelegate * GetOperationalStateDelegate();

void Shutdown();

}
}
}
}