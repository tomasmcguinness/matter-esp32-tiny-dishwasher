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

    void StartProgram();
    void StopProgram();
    void PauseProgram();
    void ResumeProgram();

    OperationalStateEnum GetOperationalState();

    uint32_t GetTimeRemaining();

    void UpdateMode(uint8_t mode);
    void SelectNextMode();
    void SelectPreviousMode();

    uint8_t GetCurrentMode();

    void TogglePower();
    void TurnOnPower();
    void TurnOffPower();
    bool IsPoweredOn();

    void ToggleProgram();
    void EndProgram();
    void ProgressProgram();

    void SetForecast();

private:
    friend DishwasherManager & DishwasherMgr(void);

    static DishwasherManager sDishwasher;

    void UpdateCurrentPhase(uint8_t phase);

    OperationalState::OperationalStateEnum mState;
    uint8_t mMode;
    uint8_t mPhase; 
    uint32_t mTimeRemaining;
    bool mIsPoweredOn = false;
};

inline DishwasherManager & DishwasherMgr(void)
{
    return DishwasherManager::sDishwasher;
}