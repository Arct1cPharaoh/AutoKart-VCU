/**
  ******************************************************************************
  * @file           : SDLogger.h
  * @brief          : Logs data in CSV format to SD card.
  ******************************************************************************
*/

#ifndef RENSSELAERMOTORSPORT_SDLOGGER_H
#define RENSSELAERMOTORSPORT_SDLOGGER_H

#include <stdint.h>

// Global flag to turn on and off this function
extern int g_sd_logging_enabled;

/**
 * @brief Mounts the SD card and prepares for logging.
 * return 0 on success, non-zero on failure.
 */
int initSdLogging();

/**
 * @brief Unmounts SD card and  safely stops logging.
 * @note Must be done before SD card removes or risk of data corruption
 */
void stopSDLogging();

/**
 * @brief Logs a single sensor or output value to the SD card
 *
 * @param name The name of the telemetry signal
 * @param value The floating-point value to log.
 */
void sdLogValue(const char* name, float value);

#endif // RENSSELAERMOTORSPORT_SDLOGGER_H
