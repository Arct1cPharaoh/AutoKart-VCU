/**
  ******************************************************************************
  * @file           : SDLogger.h
  * @brief          : Logs data in CSV format to SD card.
  ******************************************************************************
*/

#ifndef RENSSELAERMOTORSPORT_SDLOGGER_H
#define RENSSELAERMOTORSPORT_SDLOGGER_H

#include <stdint.h>

/**
 * @brief Start the SD logger task (mounts card, opens log file).
 * Creates the low-priority logger task which will mount the SD card,
 * open the CSV log file, and begin draining the internal queue.
 */
void initSdLogging();

/**
 * @brief Queue a value for logging to CSV.
 * Non-blocking. Can be called from high-rate tasks. If the logger task
 * is not ready or the queue is full, the sample is dropped.
 *
 * @param name  signal name
 * @param value signal value
 */
void sdLogValue(const char *name, float value);

#endif // RENSSELAERMOTORSPORT_SDLOGGER_H
