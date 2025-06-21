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
    void UpdateDishwasherDisplay();

    void UpdateOperationState(OperationalStateEnum state);

    OperationalStateEnum GetOperationalState();

    void UpdateMode(uint8_t mode);
    void SelectNextMode();
    void SelectPreviousMode();

private:
    friend DishwasherManager & DishwasherMgr(void);

    static DishwasherManager sDishwasher;
    
    OperationalStateEnum mState;
    uint8_t mMode;
};

inline DishwasherManager & DishwasherMgr(void)
{
    return DishwasherManager::sDishwasher;
}