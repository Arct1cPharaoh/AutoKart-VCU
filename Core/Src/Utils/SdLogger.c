#include "../../Inc/Utils/SdLogger.h"
#include "../../Inc/Utils/SdFunctions.h"
#include "stm32f7xx_hal.h"
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 128
#define LOG_LINE_MAX 128
#define LOG_QUEUE_LEN 256
#define LOGGER_TASK_DELAY_MS 1

extern RTC_HandleTypeDef hrtc;

// Logging state
typedef struct {
    char line[LOG_LINE_MAX];
} log_item_t;

static char current_log_filename[32];
static log_item_t log_queue[LOG_QUEUE_LEN];
static volatile uint16_t log_head = 0;
static volatile uint16_t log_tail = 0;

static volatile int logger_ready = 0;
static volatile uint32_t log_drops = 0;
static FIL log_file; 

// Timestamp helper
static int rtc_iso_ts(char *out, size_t out_sz) {
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d;

    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    // SubSeconds counts DOWN from SynchPrediv to 0
    uint32_t sp = hrtc.Init.SynchPrediv ? hrtc.Init.SynchPrediv : 255u;
    uint32_t ms = ((sp - t.SubSeconds) * 1000u) / (sp + 1u);

    // d.Year is 0..99 (20YY) on F7 HAL
    int year = 2000 + d.Year;

    return snprintf(out, out_sz, "%04d-%02d-%02dT%02d:%02d:%02d.%03lu",
                    year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds,
                    (unsigned long)ms);
}

// Makes a new file name each startup
static void PickNextLogFile(void)
{
    uint16_t idx = 0;
    char path[32];
    FILINFO fno;
    FRESULT fr;

    for (;;)
    {
        snprintf(path, sizeof(path), "0:/data/log_%04u.csv", idx);
        fr = f_stat(path, &fno);
        if (fr == FR_NO_FILE) {
            strncpy(current_log_filename, path, sizeof(current_log_filename) - 1);
            current_log_filename[sizeof(current_log_filename) - 1] = '\0';
            return;
        }
        idx++;
    }
}

void StopSDLogging();
void sdLogValue(const char* name, float value);

static void LoggerTask(void *argument)
{
    FRESULT fr;

    // mount card
    fr = sd_mount();
    if (fr != FR_OK) {
        vTaskDelete(NULL);
    }

    // ensure directory exists
    fr = sd_create_directory("data");
    if (fr != FR_OK && fr != FR_EXIST) {
        sd_unmount();
        vTaskDelete(NULL);
    }

    PickNextLogFile();

    // mark logger active
    logger_ready = 1;

    for (;;)
	{
		while (log_tail != log_head)
		{
			const char *line = log_queue[log_tail].line;

			fr = sd_append_file(current_log_filename, line);
			if (fr != FR_OK) {
				logger_ready = 0;
				StopSDLogging();
				vTaskDelete(NULL);
			}

			log_tail = (log_tail + 1) % LOG_QUEUE_LEN;
		}

		vTaskDelay(pdMS_TO_TICKS(LOGGER_TASK_DELAY_MS));
	}

    StopSDLogging();
}

void initSdLogging() {
    xTaskCreate(
        LoggerTask,
        "LoggerTask",
        1028,
        NULL,
        tskIDLE_PRIORITY, // lowest priority
        NULL
    );
}

/**
 * @brief Gracefully stop SD logging and unmount the card.
 * @warning Call only when producers have stopped; may block and cause faults if used concurrently.
 */
void StopSDLogging() {
    logger_ready = 0;
    f_sync(&log_file);
    f_close(&log_file);
    sd_unmount();
}

void sdLogValue(const char* name, float value) {
    if (!logger_ready) {
        return; // logger task not mounted / file not open yet
    }

    uint16_t next = (log_head + 1) % LOG_QUEUE_LEN;
    if (next == log_tail) {
        // buffer full -> drop newest
        log_drops++;
        return;
    }

    char ts[32];
    int ts_len = rtc_iso_ts(ts, sizeof(ts));
    if (ts_len <= 0) {
        return;
    }

    int n = snprintf(log_queue[log_head].line,
                     LOG_LINE_MAX,
                     "%s,%s,%.4f\n",
                     ts, name, value);

    if (n <= 0 || n >= LOG_LINE_MAX) {
        return;
    }

    log_head = next;
}

