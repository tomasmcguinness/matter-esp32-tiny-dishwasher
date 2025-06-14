/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <common_macros.h>
#include <app_priv.h>
#include <app_reset.h>

#include <app-common/zap-generated/ids/Attributes.h> // For Attribute IDs

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

static const char *TAG = "app_main";
static uint16_t dish_washer_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;
//using namespace chip::app::Clusters;

//static chip::app::Clusters::OperationalState::AppSupportedTemperatureLevelsDelegate sAppSupportedTemperatureLevelsDelegate;


static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address Changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    if (type == PRE_UPDATE) {
        /* Handle the attribute updates here. */
    }

    return ESP_OK;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    static FanDelegateImpl fan_delegate(0);
    //dish_washer_config.fan_control.delegate = &fan_delegate;

    dish_washer::config_t dish_washer_config;
    dish_washer_config.operational_state.delegate = &fan_delegate; // Set to nullptr if not using a delegate

    endpoint_t *endpoint = dish_washer::create(node, &dish_washer_config, ENDPOINT_FLAG_NONE, NULL);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create dishwasher endpoint"));

    //uint8_t phase_list[] = { 0x00, 0x01, 0x02 };

    //size_t phase_count = sizeof(phase_list) / sizeof(phase_list[0]);

    //cluster_t *operational_state_cluster = cluster::get(endpoint, OperationalState::Id);

    //esp_matter::attribute_t *phase_list_attr = esp_matter::attribute::get(operational_state_cluster, chip::app::Clusters::OperationalState::Attributes::PhaseList::Id);

    // Turn 
    //esp_matter_attr_val_t phase_val = esp_matter_array(
    //    phase_list,          // Pointer to phase array
    //    phase_count,         // Number of elements
    //    sizeof(uint8_t)      // Size of each element
    //);

    // Update the attribute
    //esp_matter::attribute::set_val(phase_list_attr, &phase_val);

    //chip::app::Clusters::OperationalState::Instance().Init();
            

    dish_washer_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Dishwasher created with endpoint_id %d", dish_washer_endpoint_id);

    // attribute_t * attribute = attribute::get(dish_washer_endpoint_id,
    //                                          chip::app::Clusters::OperationalState::Id,
    //                                          chip::app::Clusters::OperationalState::Attributes::PhaseList::Id);

    // attribute::update(endpoint_id, OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id, &val);

    //ABORT_APP_ON_FAILURE(attribute != nullptr, ESP_LOGE(TAG, "Failed to get OperationalState PhaseList attribute"));

    app_driver_init();

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    //chip::app::Clusters::TemperatureControl::SetInstance(&sAppSupportedTemperatureLevelsDelegate);

    //chip::app::Clusters::OperationalState::SetInstance(&test);

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();
#endif
}
