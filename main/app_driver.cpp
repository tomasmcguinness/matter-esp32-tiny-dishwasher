/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "bsp/esp-bsp.h"
#include <esp_log.h>
#include <app_priv.h>
#include <app-common/zap-generated/attribute-type.h>
#include "dishwasher_manager.h"

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
    ESP_LOGI(TAG, "PostAttributeChangeCallback");

    chip::app::ConcreteAttributePath info;
    info.mClusterId = Clusters::OperationalState::Id;
    info.mAttributeId = attributeId;
    info.mEndpointId = this->mEndpointId;
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

void emberAfOperationalStateClusterInitCallback(chip::EndpointId endpointId)
{
    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gOperationalStateInstance == nullptr && gOperationalStateDelegate == nullptr);

    gOperationalStateDelegate = new OperationalStateDelegate;
    EndpointId operationalStateEndpoint = 0x01;
    gOperationalStateInstance = new OperationalState::Instance(gOperationalStateDelegate, operationalStateEndpoint);

    gOperationalStateInstance->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));

    gOperationalStateInstance->Init();

    gOperationalStateDelegate->mEndpointId = endpointId;
    uint8_t value = to_underlying(OperationalStateEnum::kStopped);
    gOperationalStateDelegate->PostAttributeChangeCallback(Attributes::OperationalState::Id, ZCL_INT8U_ATTRIBUTE_TYPE, sizeof(uint8_t), &value);
}

OperationalState::Instance *OperationalState::GetInstance()
{
    return gOperationalStateInstance;
}

OperationalState::OperationalStateDelegate *OperationalState::GetDelegate()
{
    return gOperationalStateDelegate;
}

esp_err_t app_driver_init()
{
    esp_err_t err = ESP_OK;
    return err;
}
