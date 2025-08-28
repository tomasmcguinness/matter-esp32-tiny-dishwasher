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
#include <app-common/zap-generated/cluster-enums.h>
#include <app/clusters/mode-base-server/mode-base-server.h>
#include <app/clusters/mode-base-server/mode-base-cluster-objects.h>
#include <app/clusters/operational-state-server/operational-state-server.h>
#include <app/clusters/device-energy-management-server/device-energy-management-server.h>
#include <protocols/interaction_model/StatusCode.h>

typedef void *app_driver_handle_t;

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ModeBase;
using namespace chip::app::Clusters::OperationalState;
using namespace chip::app::Clusters::DishwasherMode;
using namespace chip::app::Clusters::DeviceEnergyManagement;
using namespace chip::Protocols::InteractionModel;

esp_err_t app_driver_init();

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG() \
    {                                         \
        .radio_mode = RADIO_MODE_NATIVE,      \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()               \
    {                                                      \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE, \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG() \
    {                                        \
        .storage_partition_name = "nvs",     \
        .netif_queue_size = 10,              \
        .task_queue_size = 10,               \
    }
#endif

namespace chip
{
    namespace app
    {
        namespace Clusters
        {
            namespace OperationalState
            {
                class OperationalStateDelegate : public Delegate
                {
                public:
                    uint32_t mRunningTime = 0;
                    uint32_t mPausedTime = 0;

                    app::DataModel::Nullable<uint32_t> mCountDownTime;

                    chip::app::DataModel::Nullable<uint32_t> GetCountdownTime();

                    CHIP_ERROR GetOperationalStateAtIndex(size_t index, GenericOperationalState &operationalState) override;

                    CHIP_ERROR GetOperationalPhaseAtIndex(size_t index, MutableCharSpan &operationalPhase) override;

                    void HandlePauseStateCallback(GenericOperationalError &err) override;

                    void HandleResumeStateCallback(GenericOperationalError &err) override;

                    void HandleStartStateCallback(GenericOperationalError &err) override;

                    void HandleStopStateCallback(GenericOperationalError &err) override;

                    void PostAttributeChangeCallback(AttributeId attributeId, uint8_t type, uint16_t size, uint8_t *value);

                private:
                    const GenericOperationalState opStateList[4] = {
                        GenericOperationalState(to_underlying(OperationalStateEnum::kStopped)),
                        GenericOperationalState(to_underlying(OperationalStateEnum::kRunning)),
                        GenericOperationalState(to_underlying(OperationalStateEnum::kPaused)),
                        GenericOperationalState(to_underlying(OperationalStateEnum::kError)),
                    };
                    app::DataModel::List<const GenericOperationalState> mOperationalStateList = Span<const GenericOperationalState>(opStateList);

                    const CharSpan phaseList[5] = {"pre-soak"_span, "main-wash"_span, "rinse"_span, "final-rinse"_span, "drying"_span};
                    Span<const CharSpan> mOperationalPhaseList = Span<const CharSpan>(phaseList);
                };

                OperationalState::Instance *GetInstance();
                OperationalState::OperationalStateDelegate *GetDelegate();

                void Shutdown();

            } // namespace OperationalState
        } // namespace Clusters
    } // namespace app
} // namespace chip

namespace chip
{
    namespace app
    {
        namespace Clusters
        {
            namespace DishwasherMode
            {
                const uint8_t ModeNormal = 0;
                const uint8_t ModeHeavy = 1;
                const uint8_t ModeLight = 2;

                class DishwasherModeDelegate : public ModeBase::Delegate
                {
                private:
                    using ModeTagStructType = detail::Structs::ModeTagStruct::Type;
                    ModeTagStructType modeTagsNormal[1] = {{.value = to_underlying(ModeTag::kNormal)}};
                    ModeTagStructType modeTagsHeavy[2] = {{.value = to_underlying(ModeBase::ModeTag::kMax)},
                                                          {.value = to_underlying(ModeTag::kHeavy)}};
                    ModeTagStructType modeTagsLight[3] = {{.value = to_underlying(ModeTag::kLight)},
                                                          {.value = to_underlying(ModeBase::ModeTag::kNight)},
                                                          {.value = to_underlying(ModeBase::ModeTag::kQuiet)}};

                    // These are the modes on my dishwasher.
                    // TODO Implement these with the appropriate ModeTags.
                    //
                    // Eco 50°
                    // Chef 70°
                    // Auto 45° - 65°
                    // Glass 40°
                    // Silence 50°
                    // Pre Rinse
                    // Quick 45°
                    // Short 60°
                    // Machine Care

                    const detail::Structs::ModeOptionStruct::Type kModeOptions[3] = {
                        detail::Structs::ModeOptionStruct::Type{.label = CharSpan::fromCharString("Eco 50°"),
                                                                .mode = ModeNormal,
                                                                .modeTags = DataModel::List<const ModeTagStructType>(modeTagsNormal)},
                        detail::Structs::ModeOptionStruct::Type{.label = CharSpan::fromCharString("Chef 70°"),
                                                                .mode = ModeHeavy,
                                                                .modeTags = DataModel::List<const ModeTagStructType>(modeTagsHeavy)},
                        detail::Structs::ModeOptionStruct::Type{.label = CharSpan::fromCharString("Quick 45°"),
                                                                .mode = ModeLight,
                                                                .modeTags = DataModel::List<const ModeTagStructType>(modeTagsLight)}};

                    CHIP_ERROR Init() override;
                    void HandleChangeToMode(uint8_t mode, ModeBase::Commands::ChangeToModeResponse::Type &response) override;

                    CHIP_ERROR GetModeValueByIndex(uint8_t modeIndex, uint8_t &value) override;
                    CHIP_ERROR GetModeTagsByIndex(uint8_t modeIndex, DataModel::List<ModeTagStructType> &tags) override;

                public:
                    ~DishwasherModeDelegate() override = default;

                    CHIP_ERROR GetModeLabelByIndex(uint8_t modeIndex, MutableCharSpan &label) override;

                    void PostAttributeChangeCallback(AttributeId attributeId, uint8_t type, uint16_t size, uint8_t *value);

                    Protocols::InteractionModel::Status SetDishwasherMode(uint8_t mode);
                };

                ModeBase::Instance *GetInstance();
                ModeBase::Delegate *GetDelegate();

                void Shutdown();

            } // namespace DishwasherMode

        } // namespace Clusters
    } // namespace app
} // namespace chip

namespace chip
{
    namespace app
    {
        namespace Clusters
        {
            namespace DeviceEnergyManagement
            {
                class DeviceEnergyManagementDelegate : public DeviceEnergyManagement::Delegate
                {
                public:

                    DeviceEnergyManagementDelegate()
                    { 
                        // Left as an example of constructor
                        ESP_LOGI("DeviceEnergyManagementDelegate", "Constructing");
                    }

                    Status PowerAdjustRequest(const int64_t powerMw, const uint32_t durationS, AdjustmentCauseEnum cause);
                    Status CancelPowerAdjustRequest();
                    Status StartTimeAdjustRequest(const uint32_t requestedStartTime, AdjustmentCauseEnum cause);
                    Status PauseRequest(const uint32_t duration, AdjustmentCauseEnum cause);
                    Status ResumeRequest();
                    Status ModifyForecastRequest(const uint32_t forecastID, const DataModel::DecodableList<Structs::SlotAdjustmentStruct::Type> &slotAdjustments, AdjustmentCauseEnum cause);
                    Status RequestConstraintBasedForecast(const DataModel::DecodableList<Structs::ConstraintsStruct::Type> &constraints, AdjustmentCauseEnum cause);
                    Status CancelRequest();

                    ESATypeEnum GetESAType();
                    bool GetESACanGenerate();
                    ESAStateEnum GetESAState();
                    int64_t GetAbsMinPower();
                    int64_t GetAbsMaxPower();
                    OptOutStateEnum GetOptOutState();
                    void SetOptOutState(OptOutStateEnum state);

                    CHIP_ERROR SetESAState(ESAStateEnum newValue);

                    chip::app::DataModel::Nullable<DeviceEnergyManagement::Structs::PowerAdjustCapabilityStruct::Type> &GetPowerAdjustmentCapability() override;
                    chip::app::DataModel::Nullable<DeviceEnergyManagement::Structs::ForecastStruct::Type> &GetForecast() override;

                    CHIP_ERROR SetForecast(const chip::app::DataModel::Nullable<DeviceEnergyManagement::Structs::ForecastStruct::Type> &);

                    ~DeviceEnergyManagementDelegate() override = default;

                private:
                    chip::app::DataModel::Nullable<DeviceEnergyManagement::Structs::PowerAdjustCapabilityStruct::Type> mPowerAdjustCapabilityStruct;
                    chip::app::DataModel::Nullable<DeviceEnergyManagement::Structs::ForecastStruct::Type> mForecast;
                    OptOutStateEnum mOptOutState = OptOutStateEnum::kOptOut;
                };

                void Shutdown();
            }
        }
    }
}

extern chip::app::Clusters::DeviceEnergyManagement::DeviceEnergyManagementDelegate device_energy_management_delegate;