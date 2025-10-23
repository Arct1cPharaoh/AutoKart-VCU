#include "../../Inc/Utils/MessageFormat.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Telemetry output method selection
#define TELEMETRY_DISABLED  0
#define TELEMETRY_UART      1  
#define TELEMETRY_USB_CDC   2

// Change this to switch telemetry output method
#define TELEMETRY_METHOD TELEMETRY_USB_CDC

#if TELEMETRY_METHOD == TELEMETRY_USB_CDC
// Include USB CDC when available
#include "../../../USB_DEVICE/App/usbd_cdc_if.h"
#endif

static const char* getTypeString(MessageType type) {
    const char* type_strings[] = {
        "SENSOR_VALUE",
        "OUTPUT_VALUE", 
        "SYSTEM_STATUS",
        "WARNING",
        "ERROR", 
        "DEBUG",
        "CAN_TX",
        "CAN_RX",
        "TIMER_STATS",
        "CONFIG"
    };
    return type_strings[type];
}

void sendMessage(const char* sender, MessageType type, const char* format, ...) {
#if TELEMETRY_METHOD == TELEMETRY_DISABLED
    // Telemetry disabled for maximum performance
    return;
    
#elif TELEMETRY_METHOD == TELEMETRY_UART
    // Original UART method
    printf("Sender:%s;InfoType:%s;Content:", sender, getTypeString(type));
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\r\n");
    
#elif TELEMETRY_METHOD == TELEMETRY_USB_CDC
    // USB CDC method - 100x faster than UART
    char buffer[256];
    
    // Format complete message in buffer
    int offset = snprintf(buffer, 100, "Sender:%s;InfoType:%s;Content:", 
                         sender, getTypeString(type));
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + offset, 150, format, args);
    va_end(args);
    
    strcat(buffer, "\r\n");
    
    // Send via USB CDC - 100x faster than UART!
    CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
    
#endif
}