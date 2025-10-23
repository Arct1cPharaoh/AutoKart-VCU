#include "../../../Inc/Utils/Common.h"
#include "../../../Inc/Utils/MessageFormat.h"
#include "../../../Inc/Systems/External/Inverter.h"

#include <string.h>  // For memset
#include <stdint.h>  // For uint32_t

// Forward declaration for HAL function
extern uint32_t HAL_GetTick(void);

void initInverter(Inverter* inverter, TorqueControl* tc, int hz, int maxCurrent, int maxTemp, int maxVoltage) {
    initExternalSystem(&inverter->base, "Inverter", hz, e_INVERTER, updateInverter, checkInverterHeartbeat, inverter);
    inverter->tc = tc;
    inverter->maxCurrent = maxCurrent;
    inverter->maxTemp = maxTemp;
    inverter->maxVoltage = maxVoltage;
    
    // Initialize CAN data structure (using auto-generated DBC structures)
    memset(&inverter->can_data.status1, 0, sizeof(inverter->can_data.status1));
    memset(&inverter->can_data.status2, 0, sizeof(inverter->can_data.status2));
    memset(&inverter->can_data.control, 0, sizeof(inverter->can_data.control));
    inverter->can_data.new_data_flags = 0;
    inverter->can_data.last_update = 0;
    inverter->can_data.message_count = 0;
}

int updateInverter(ExternalSystem* external) {
    Inverter* inverter = (Inverter*)external->child;
    
    // Process new CAN data if available
    if (inverter->can_data.new_data_flags != 0) {
        if (inverter->can_data.new_data_flags & INVERTER_NEW_SPEED) {
            sendMessage("INVERTER", MSG_SYSTEM_STATUS, 
                       "NewMotorSpeed=%d RPM", inverter->can_data.status1.motor_speed);
        }
        
        if (inverter->can_data.new_data_flags & INVERTER_NEW_TEMP) {
            // Apply DBC offset to get real temperature
            float motor_temp = inverter->can_data.status1.motor_temperature - 40.0f;
            if (motor_temp > inverter->maxTemp) {
                sendMessage("INVERTER", MSG_WARNING, 
                           "MotorTempHigh=%f degC (limit=%d)", 
                           motor_temp, inverter->maxTemp);
            }
        }
        
        if (inverter->can_data.new_data_flags & INVERTER_NEW_CURRENT) {
            // Apply DBC scale to get real current
            float motor_current = inverter->can_data.status2.motor_current * 0.1f;
            if (motor_current > inverter->maxCurrent) {
                sendMessage("INVERTER", MSG_WARNING, 
                           "MotorCurrentHigh=%f A (limit=%d)", 
                           motor_current, inverter->maxCurrent);
            }
        }
        
        // Clear the flags after processing
        inverter->can_data.new_data_flags = 0;
    }
    
    // Check for CAN communication timeout (5 seconds)
    if (isInverterCANDataStale(inverter, 5000)) {
        sendMessage("INVERTER", MSG_ERROR, "CAN communication timeout");
        // Could enter safe mode here
    }
    
    // Check if torque control is validated
    if (inverter->tc->base.state != c_validated) {
        //printf("Inverter: Torque Control Actuator is not validated\r\n");
        return _FAILURE;
    }

    
    #ifdef DEBUGn
    //printf("Inverter updated. Torque: %f, Current: %d, Temp: %d, Voltage: %d\r\n",
           inverter->tc->desiredTorque, inverter->maxCurrent, inverter->maxTemp, inverter->maxVoltage);
    #endif
    
    dac1_buffer[0] = ((inverter->tc->desiredTorque) / (inverter->tc->maxAllowedTorque)) * 4096.0;

    return _SUCCESS;
}

int checkInverterHeartbeat(void* self) {
    (void)self;  // Unused parameter
    // TODO: Check if the inverter is still alive
    return _SUCCESS;
}

// CAN Data Access API Implementation

int hasNewInverterCANData(Inverter* inverter) {
    return inverter->can_data.new_data_flags != 0;
}

float getInverterMotorSpeed(Inverter* inverter) {
    return (float)inverter->can_data.status1.motor_speed;  // Direct RPM value
}

float getInverterMotorTorque(Inverter* inverter) {
    return inverter->can_data.status1.motor_torque * 0.1f;  // Apply DBC scale factor
}

float getInverterMotorTemp(Inverter* inverter) {
    return inverter->can_data.status1.motor_temperature - 40.0f;  // Apply DBC offset
}

float getInverterMotorVoltage(Inverter* inverter) {
    return inverter->can_data.status2.motor_voltage * 0.1f;  // Apply DBC scale factor
}

float getInverterMotorCurrent(Inverter* inverter) {
    return inverter->can_data.status2.motor_current * 0.1f;  // Apply DBC scale factor
}

uint32_t getInverterDataAge(Inverter* inverter) {
    if (inverter->can_data.last_update == 0) {
        return UINT32_MAX; // No data received yet
    }
    return HAL_GetTick() - inverter->can_data.last_update;
}

int isInverterCANDataStale(Inverter* inverter, uint32_t timeout_ms) {
    return getInverterDataAge(inverter) > timeout_ms;
}

void clearInverterNewDataFlags(Inverter* inverter) {
    inverter->can_data.new_data_flags = 0;
}
