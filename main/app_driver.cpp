/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <app_priv.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app/util/generic-callbacks.h>
#include <protocols/interaction_model/StatusCode.h>
#include "dishwasher_manager.h"
#include <esp_debug_helpers.h>
#include <iot_button.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;

static const char *TAG = "app_driver";

DataModel::Nullable<uint32_t> OperationalStateDelegate::GetCountdownTime()
{
    return DataModel::MakeNullable((uint32_t)0);
}

CHIP_ERROR OperationalStateDelegate::GetOperationalStateAtIndex(size_t index, GenericOperationalState &operationalState)
{
    ESP_LOGI(TAG, "GetOperationalStateAtIndex");

    if (index > mOperationalStateList.size() - 1)
    {
        return CHIP_ERROR_NOT_FOUND;
    }

    operationalState = mOperationalStateList[index];

    return CHIP_NO_ERROR;
}

CHIP_ERROR OperationalStateDelegate::GetOperationalPhaseAtIndex(size_t index, MutableCharSpan &operationalPhase)
{
    ESP_LOGI(TAG, "GetOperationalPhaseAtIndex");

    if (index == 0)
    {
        chip::CopyCharSpanToMutableCharSpan(CharSpan::fromCharString("Warming Water"), operationalPhase);
        return CHIP_NO_ERROR;
    }
    else
    {
        return CHIP_ERROR_NOT_FOUND;
    }
}

void OperationalStateDelegate::HandlePauseStateCallback(GenericOperationalError &err)
{
    ESP_LOGI(TAG, "HandlePauseStateCallback");
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kPaused));

    if (error == CHIP_NO_ERROR)
    {
        GetInstance()->UpdateCountdownTimeFromDelegate();
        DishwasherMgr().UpdateOperationState(OperationalStateEnum::kPaused);
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void OperationalStateDelegate::HandleResumeStateCallback(GenericOperationalError &err)
{
    ESP_LOGI(TAG, "HandleResumeStateCallback");
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kRunning));

    if (error == CHIP_NO_ERROR)
    {
        GetInstance()->UpdateCountdownTimeFromDelegate();
        DishwasherMgr().UpdateOperationState(OperationalStateEnum::kRunning);
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void OperationalStateDelegate::HandleStartStateCallback(GenericOperationalError &err)
{
    ESP_LOGI(TAG, "HandleStartStateCallback");
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kRunning));

    if (error == CHIP_NO_ERROR)
    {
        GetInstance()->UpdateCountdownTimeFromDelegate();
        DishwasherMgr().UpdateOperationState(OperationalStateEnum::kRunning);
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void OperationalStateDelegate::HandleStopStateCallback(GenericOperationalError &err)
{
    ESP_LOGI(TAG, "HandleStopStateCallback");
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kStopped));

    if (error == CHIP_NO_ERROR)
    {
        GetInstance()->UpdateCountdownTimeFromDelegate();
        DishwasherMgr().UpdateOperationState(OperationalStateEnum::kStopped);
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void OperationalStateDelegate::PostAttributeChangeCallback(AttributeId attributeId, uint8_t type, uint16_t size, uint8_t *value)
{
    ESP_LOGI(TAG, "OperationalStateDelegate::PostAttributeChangeCallback");

    // chip::app::ConcreteAttributePath info;
    // info.mClusterId = Clusters::OperationalState::Id;
    // info.mAttributeId = attributeId;
    // info.mEndpointId = this->mEndpointId;
    // MatterPostAttributeChangeCallback(info,type, size, value);
}

static OperationalState::Instance *gOperationalStateInstance = nullptr;
static OperationalStateDelegate *gOperationalStateDelegate = nullptr;

void OperationalState::Shutdown()
{
    if (gOperationalStateInstance != nullptr)
    {
        delete gOperationalStateInstance;
        gOperationalStateInstance = nullptr;
    }
    if (gOperationalStateDelegate != nullptr)
    {
        delete gOperationalStateDelegate;
        gOperationalStateDelegate = nullptr;
    }
}

OperationalState::OperationalStateDelegate *OperationalState::GetDelegate()
{
    return gOperationalStateDelegate;
}

void emberAfOperationalStateClusterInitCallback(chip::EndpointId endpointId)
{
    ESP_LOGI(TAG, "emberAfOperationalStateClusterInitCallback()");

    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gOperationalStateInstance == nullptr && gOperationalStateDelegate == nullptr);

    gOperationalStateDelegate = new OperationalStateDelegate;
    EndpointId operationalStateEndpoint = 0x01;
    gOperationalStateInstance = new OperationalState::Instance(gOperationalStateDelegate, operationalStateEndpoint);

    gOperationalStateInstance->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));

    gOperationalStateInstance->Init();

    gOperationalStateDelegate->mEndpointId = endpointId;
    uint8_t value = to_underlying(OperationalStateEnum::kStopped);
    gOperationalStateDelegate->PostAttributeChangeCallback(chip::app::Clusters::OperationalState::Attributes::OperationalState::Id, ZCL_INT8U_ATTRIBUTE_TYPE, sizeof(uint8_t), &value);
}

//****************************
//* DISHWASHER MODE DELEGATE *
//****************************

using chip::Protocols::InteractionModel::Status;
template <typename T>
using List              = chip::app::DataModel::List<T>;
using ModeTagStructType = chip::app::Clusters::detail::Structs::ModeTagStruct::Type;

static DishwasherModeDelegate * gDishwasherModeDelegate = nullptr;
static ModeBase::Instance * gDishwasherModeInstance     = nullptr;

CHIP_ERROR DishwasherModeDelegate::Init()
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::Init()");

    gDishwasherModeInstance = mInstance;

    return CHIP_NO_ERROR;
}

// todo refactor code by making a parent class for all ModeInstance classes to reduce flash usage.
void DishwasherModeDelegate::HandleChangeToMode(uint8_t NewMode, ModeBase::Commands::ChangeToModeResponse::Type & response)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::HandleChangeToMode()");
    DishwasherManager().UpdateMode(NewMode);
    response.status = to_underlying(ModeBase::StatusCode::kSuccess);
}

CHIP_ERROR DishwasherModeDelegate::GetModeLabelByIndex(uint8_t modeIndex, chip::MutableCharSpan & label)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::GetModeLabelByIndex()");
    if (modeIndex >= MATTER_ARRAY_SIZE(kModeOptions))
    {
        ESP_LOGI(TAG, "CHIP_ERROR_PROVIDER_LIST_EXHAUSTED");
        return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
    }
    return chip::CopyCharSpanToMutableCharSpan(kModeOptions[modeIndex].label, label);
}

CHIP_ERROR DishwasherModeDelegate::GetModeValueByIndex(uint8_t modeIndex, uint8_t & value)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::GetModeValueByIndex(%d)", modeIndex);

    if (modeIndex >= MATTER_ARRAY_SIZE(kModeOptions))
    {
        ESP_LOGI(TAG, "CHIP_ERROR_PROVIDER_LIST_EXHAUSTED");
        return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
    }
    value = kModeOptions[modeIndex].mode;

    ESP_LOGI(TAG, "DishwasherModeDelegate::GetModeValueByIndex - Returning value %d for modeIndex: %d", value, modeIndex);

    return CHIP_NO_ERROR;
}

CHIP_ERROR DishwasherModeDelegate::GetModeTagsByIndex(uint8_t modeIndex, List<ModeTagStructType> & tags)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::GetModeTagsByIndex()");
    if (modeIndex >= MATTER_ARRAY_SIZE(kModeOptions))
    {
        return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
    }

    if (tags.size() < kModeOptions[modeIndex].modeTags.size())
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    std::copy(kModeOptions[modeIndex].modeTags.begin(), kModeOptions[modeIndex].modeTags.end(), tags.begin());
    tags.reduce_size(kModeOptions[modeIndex].modeTags.size());

    return CHIP_NO_ERROR;
}

Status DishwasherModeDelegate::SetDishwasherMode(uint8_t modeValue)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::SetDishwasherMode");

    // We can only update the DishwasherMode when it's not running.
    //

    VerifyOrReturnError(DishwasherMode::GetInstance() != nullptr, Status::InvalidInState);

    if (!DishwasherMode::GetInstance()->IsSupportedMode(modeValue))
    {
        ChipLogError(AppServer, "SetDishwasherMode bad mode");
        return Status::ConstraintError;
    }

    Status status = DishwasherMode::GetInstance()->UpdateCurrentMode(modeValue);
    if (status != Status::Success)
    {
        ChipLogError(AppServer, "SetDishwasherMode updateMode failed 0x%02x", to_underlying(status));
        return status;
    }

    return chip::Protocols::InteractionModel::Status::Success;
}

void DishwasherModeDelegate::PostAttributeChangeCallback(AttributeId attributeId, uint8_t type, uint16_t size, uint8_t *value)
{
    ESP_LOGI(TAG, "DishwasherModeDelegate::PostAttributeChangeCallback");
}

ModeBase::Instance * DishwasherMode::GetInstance()
{
    return gDishwasherModeInstance;
}

ModeBase::Delegate * DishwasherMode::GetDelegate()
{
    return gDishwasherModeDelegate;
}

void DishwasherMode::Shutdown()
{
    if (gDishwasherModeInstance != nullptr)
    {
        delete gDishwasherModeInstance;
        gDishwasherModeInstance = nullptr;
    }
    if (gDishwasherModeDelegate != nullptr)
    {
        delete gDishwasherModeDelegate;
        gDishwasherModeDelegate = nullptr;
    }
}

void emberAfDishwasherModeClusterInitCallback(chip::EndpointId endpointId)
{
    ESP_LOGI(TAG, "emberAfDishwasherModeClusterInitCallback()");

    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gDishwasherModeDelegate == nullptr && gDishwasherModeInstance == nullptr);
    gDishwasherModeDelegate = new DishwasherMode::DishwasherModeDelegate;
    // TODO Restore the deadfront support by setting the OnOff feature.
    //gDishwasherModeInstance = new ModeBase::Instance(gDishwasherModeDelegate, 0x1, DishwasherMode::Id, chip::to_underlying(chip::app::Clusters::DishwasherMode:: ::Feature::kOnOff));
    gDishwasherModeInstance = new ModeBase::Instance(gDishwasherModeDelegate, 0x1, DishwasherMode::Id, 0);
    gDishwasherModeInstance->Init();

    uint8_t currentMode = gDishwasherModeInstance->GetCurrentMode();

    ESP_LOGI(TAG, "CurrentMode: %d", currentMode);
}

// static void app_driver_button_press_up_cb(void *arg, void *data)
// {
//     ESP_LOGI(TAG, "Button Press Up");

//     if (iot_button_get_ticks_time((button_handle_t)arg) < 1000)
//     {
//         ESP_LOGI(TAG, "Single Click");
//         client::request_handle_t req_handle;
//         req_handle.type = esp_matter::client::INVOKE_CMD;
//         req_handle.command_path.mClusterId = OnOff::Id;
//         req_handle.command_path.mCommandId = OnOff::Commands::Toggle::Id;

//         // lock::chip_stack_lock(portMAX_DELAY);
//         // client::cluster_update(switch_endpoint_id, &req_handle);
//         // lock::chip_stack_unlock();
//     }
//     else
//     {
//         ESP_LOGI(TAG, "Long Press");
//         swap_dimmer_direction();
//     }
// }

// static void app_driver_button_dimming_cb(void *arg, void *data)
// {
//     ESP_LOGI(TAG, "Long Press Hold");

//     uint16_t hold_count = iot_button_get_long_press_hold_cnt((button_handle_t)arg);
//     uint32_t hold_time = iot_button_get_ticks_time((button_handle_t)arg);

//     ESP_LOGI(TAG, "Long Press Hold Count: %d", hold_count);
//     ESP_LOGI(TAG, "Long Press Hold Time: %ld", hold_time);

//     LevelControl::Commands::Step::Type stepCommand;
//     stepCommand.stepMode = current_step_direction;
//     stepCommand.stepSize = 3;
//     stepCommand.transitionTime.SetNonNull(0);
//     stepCommand.optionsMask = static_cast<chip::BitMask<chip::app::Clusters::LevelControl::LevelControlOptions>>(0U);
//     stepCommand.optionsOverride = static_cast<chip::BitMask<chip::app::Clusters::LevelControl::LevelControlOptions>>(0U);

//     client::request_handle_t req_handle;
//     req_handle.type = esp_matter::client::INVOKE_CMD;
//     req_handle.command_path.mClusterId = LevelControl::Id;
//     req_handle.command_path.mCommandId = LevelControl::Commands::Step::Id;
//     req_handle.request_data = &stepCommand;

//     lock::chip_stack_lock(portMAX_DELAY);
//     client::cluster_update(switch_endpoint_id, &req_handle);
//     lock::chip_stack_unlock();
// }

// static void app_driver_button_long_press_up_cb(void *arg, void *data)
// {
//     ESP_LOGI(TAG, "Long Press Up");
//     //swap_dimmer_direction();
// }

static void app_driver_button_long_press_start_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Long Press Started");
    DishwasherMgr().TogglePower();
}

esp_err_t app_driver_init()
{
    esp_err_t err = ESP_OK;

    // Create an OnOff Button.
    //
    button_config_t config;
    memset(&config, 0, sizeof(button_config_t));

    config.type = BUTTON_TYPE_GPIO;
    config.gpio_button_config.gpio_num = GPIO_NUM_0;
    config.gpio_button_config.active_level = 1;

    button_handle_t handle = iot_button_create(&config);

    ESP_ERROR_CHECK(iot_button_register_cb(handle, BUTTON_LONG_PRESS_START, app_driver_button_long_press_start_cb, NULL));
    //ESP_ERROR_CHECK(iot_button_register_cb(handle, BUTTON_LONG_PRESS_START, app_driver_button_long_press_start_cb, NULL));
    //ESP_ERROR_CHECK(iot_button_register_cb(handle, BUTTON_LONG_PRESS_HOLD, app_driver_button_long_press_hold_cb, NULL));

    // client::set_request_callback(app_driver_client_invoke_command_callback,
    //                              app_driver_client_group_invoke_command_callback, NULL);

    return err;
}