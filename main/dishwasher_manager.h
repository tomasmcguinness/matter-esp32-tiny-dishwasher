#include <lib/core/CHIPError.h>
#include <app/clusters/operational-state-server/operational-state-server.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;

class DishwasherManager
{
public:
    esp_err_t Init();
    void UpdateDishwasherLed();
    void UpdateOperationState(OperationalStateEnum state);
    OperationalStateEnum GetOperationalState();

private:
    friend DishwasherManager & DishwasherMgr(void);
    OperationalStateEnum mState;
    static DishwasherManager sDishwasher;
};

inline DishwasherManager & DishwasherMgr(void)
{
    return DishwasherManager::sDishwasher;
}