#include "../Test/test.h"
#include "../../Inc/Systems/System.h"
#include "../../Inc/Systems/ControllerSystem.h"
#include "../../Inc/Systems/MonitorSystem.h"

static int mock_system_init_calls = 0;
static int mock_system_update_calls = 0;

static void mock_system_init(void* data) {
    mock_system_init_calls++;
}

static void mock_system_update(void* data) {
    mock_system_update_calls++;
}

void test_system_init(void) {
    printf("  🔧 Testing system initialization...\n");
    
    mock_system_init_calls = 0;
    
    // Suppress debug output from SystemInit
    TEST_SILENT_START();
    System system;
    SystemInit(&system, "TestSystem", 10, mock_system_init, mock_system_update, NULL);
    TEST_SILENT_END();
    
    TEST_ASSERT_STR_EQ("TestSystem", system.base.name, "System name set");
    TEST_ASSERT_EQ(10, system.base.hz, "System frequency set");
    TEST_ASSERT_EQ(1, mock_system_init_calls, "System init called");
    
    printf("  ✅ System initialization test completed\n");
}

void test_system_update(void) {
    printf("  🔧 Testing system update...\n");
    
    mock_system_update_calls = 0;
    
    TEST_SILENT_START();
    System system;
    SystemInit(&system, "TestSystem", 10, mock_system_init, mock_system_update, NULL);
    
    // Trigger update
    system.base.update(system.base.data);
    TEST_SILENT_END();
    
    TEST_ASSERT_EQ(1, mock_system_update_calls, "System update called");
    
    printf("  ✅ System update test completed\n");
}

void system_main(void) {
    TEST_SUITE_START("System");
    
    test_system_init();
    test_system_update();
    
    TEST_SUITE_END();
}