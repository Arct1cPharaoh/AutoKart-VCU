#ifndef RENNSSELAERMOTORSPORT_INVERTER_H
#define RENNSSELAERMOTORSPORT_INVERTER_H

#include "../ExternalSystem.h"
#include "../Controller/TorqueControl.h"
#include "../../Utils/Constants.h"
#include "../../Files/inverter_test_dbc.h"

// CAN Data flags for new data detection
#define INVERTER_NEW_SPEED      (1 << 0)
#define INVERTER_NEW_TORQUE     (1 << 1)
#define INVERTER_NEW_TEMP       (1 << 2)
#define INVERTER_NEW_VOLTAGE    (1 << 3)
#define INVERTER_NEW_CURRENT    (1 << 4)

typedef struct {
    ExternalSystem base;
    TorqueControl* tc;
    int maxCurrent;
    int maxTemp;
    int maxVoltage;
    
    // CAN Data Storage - using auto-generated DBC structures
    struct {
        ESC_Motor_Status_1_t status1;    // Motor speed, torque, temp, fault status
        ESC_Motor_Status_2_t status2;    // Motor voltage, current, power, ESC temp
        ESC_Control_Status_t control;    // ESC state, enables, limits
        uint8_t new_data_flags;          // Bit flags indicating new data
        uint32_t last_update;            // Timestamp of last CAN update
        uint32_t message_count;          // Total messages received
    } can_data;
} Inverter;

extern uint32_t dac1_buffer[DAC1_BUFFER_SIZE];

/**
 * @brief Initializes the Inverter with initial settings.
 * 
 * @param inverter A pointer to the Inverter structure.
 * @param tc A pointer to the TorqueControl structure.
 * @param hz Rate at which the inverter is called (in hz).
 * @param maxCurrent The maximum current limit set for the inverter.
 * @param maxTemp The maximum temperature limit set for the inverter.
 * @param maxVoltage The maximum voltage limit set for the inverter.
 */
void initInverter(Inverter* inverter, TorqueControl* tc, int hz, int maxCurrent, int maxTemp, int maxVoltage);

/**
 * @brief Starts the Inverter.
 * 
 * @param inverter A pointer to the Inverter structure.
 * @return int _SUCCESS or _FAILURE.
 */
int startInverter(Inverter* inverter);

/**
 * @brief Updates the Inverter.
 * 
 * @param external A pointer to the Inverter ExternalSystem.
 * @return int _SUCCESS or _FAILURE.
 */
int updateInverter(ExternalSystem* external);

/** 
 * @brief Checks the heartbeat of the Inverter.
 * 
 * @param self A pointer to the Inverter structure.
 * @return int _SUCCESS or _FAILURE.
 */
int checkInverterHeartbeat(void* self);

/**
 * @brief CAN Data Access API
 */

// Check if new CAN data is available
int hasNewInverterCANData(Inverter* inverter);

// Get latest motor parameters (whether new or cached)
float getInverterMotorSpeed(Inverter* inverter);    // RPM
float getInverterMotorTorque(Inverter* inverter);   // Nm
float getInverterMotorTemp(Inverter* inverter);     // degC
float getInverterMotorVoltage(Inverter* inverter);  // V
float getInverterMotorCurrent(Inverter* inverter);  // A

// Get data freshness information
uint32_t getInverterDataAge(Inverter* inverter);    // milliseconds since last update
int isInverterCANDataStale(Inverter* inverter, uint32_t timeout_ms);  // 1 if stale, 0 if fresh

// Clear new data flags (called after processing)
void clearInverterNewDataFlags(Inverter* inverter);

#endif // RENNSSELAERMOTORSPORT_INVERTER_H
