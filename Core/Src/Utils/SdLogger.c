#include "../../Inc/Utils/SdLogger.h"
#include "../../Inc/Utils/SdFunctions.h"
#include "stm32f7xx_hal.h"
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 128
#define LOG_FILENAME "datalog.csv"

extern RTC_HandleTypeDef hrtc;

int g_sd_logging_enabled = 1;

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

int initSdLogging() {
    if (sd_mount() != FR_OK) {
        g_sd_logging_enabled = 0;
        return -1;
    }

    g_sd_logging_enabled = 1;
    return 0;
}

void StopSDLogging() {
    g_sd_logging_enabled = 0;
    sd_unmount();
}

void sdLogValue(const char *name, float value)
{
    if (!g_sd_logging_enabled) {
        printf("[sdLogValue] Logging disabled\r\n");
        return;
    }

    char ts[32];
    int ts_len = rtc_iso_ts(ts, sizeof(ts));
    if (ts_len <= 0) {
        printf("[sdLogValue] RTC failed\r\n");
        return;
    }

    char line[BUF_SIZE];
    int n = snprintf(line, sizeof(line), "%s,%s,%.4f\n", ts, name, value);
    if (n <= 0 || n >= (int)sizeof(line)) {
        printf("[sdLogValue] snprintf error (n=%d)\r\n", n);
        return;
    }

    printf("[sdLogValue] Writing line: %s", line);
    printf("[sdLogValue] Mount check: fs type=%u\r\n", fs.fs_type);  // <== add this
    printf("[sdLogValue] Trying append: '%s'\r\n", LOG_FILENAME);

    FRESULT res = sd_append_file(LOG_FILENAME, line);
    printf("[sdLogValue] sd_append_file -> %d\r\n", res);
}

