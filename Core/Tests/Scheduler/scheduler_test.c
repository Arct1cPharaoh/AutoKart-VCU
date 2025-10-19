#include "../Test/test.h"
#include "../../Inc/Scheduler/Scheduler.h"
#include "../../Inc/Utils/Updateable.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Mock updateables for testing
static int mock_update_count = 0;
static int mock_sensor1_calls = 0;
static int mock_sensor2_calls = 0;

// Update function signature should match Updateable's expected signature
static int mock_sensor1_update(Updateable* updateable) {
    mock_sensor1_calls++;
    mock_update_count++;
    printf("Mock sensor 1 update called (total: %d)\n", mock_sensor1_calls);
    return 0; // Return success
}

static int mock_sensor2_update(Updateable* updateable) {
    mock_sensor2_calls++;
    mock_update_count++;
    printf("Mock sensor 2 update called (total: %d)\n", mock_sensor2_calls);
    return 0; // Return success
}

static Updateable test_sensor1 = {
    .name = "TestSensor1",
    .hz = 10,  // 10 Hz
    .update = mock_sensor1_update
    // Remove .data = NULL since it doesn't exist in your Updateable structure
};

static Updateable test_sensor2 = {
    .name = "TestSensor2", 
    .hz = 5,   // 5 Hz
    .update = mock_sensor2_update
    // Remove .data = NULL since it doesn't exist in your Updateable structure
};

static Updateable* test_updateables[] = {
    &test_sensor1,
    &test_sensor2,
    NULL
};

void test_scheduler_init(void) {
    Scheduler scheduler;
    
    SchedulerInit(&scheduler, test_updateables);
    
    TEST_ASSERT_EQ(0, scheduler.running, "Scheduler not running after init");
    
    SchedulerCleanup();
}

void test_scheduler_run_stop(void) {
    Scheduler scheduler;
    mock_update_count = 0;
    mock_sensor1_calls = 0;
    mock_sensor2_calls = 0;
    
    SchedulerInit(&scheduler, test_updateables);
    
    // Start scheduler in separate thread
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void*)SchedulerRun, &scheduler);
    
    // Let it run for 1 second
    sleep(1);
    
    SchedulerStop(&scheduler);
    pthread_join(scheduler_thread, NULL);
    
    // Check that updates were called
    TEST_ASSERT(mock_sensor1_calls >= 8 && mock_sensor1_calls <= 12, 
               "Sensor1 called approximately 10 times per second");
    TEST_ASSERT(mock_sensor2_calls >= 3 && mock_sensor2_calls <= 7,
               "Sensor2 called approximately 5 times per second");
    TEST_ASSERT(mock_update_count > 0, "At least some updates occurred");
    
    SchedulerCleanup();
}

void test_scheduler_suspend_resume(void) {
    Scheduler scheduler;
    mock_sensor1_calls = 0;
    
    SchedulerInit(&scheduler, test_updateables);
    
    // Start scheduler
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void*)SchedulerRun, &scheduler);
    
    // Run briefly
    usleep(200000); // 200ms
    int calls_before_suspend = mock_sensor1_calls;
    
    // Suspend sensor1
    SchedulerSuspendUpdateable("TestSensor1");
    usleep(200000); // 200ms
    int calls_after_suspend = mock_sensor1_calls;
    
    // Resume sensor1
    SchedulerResumeUpdateable("TestSensor1");
    usleep(200000); // 200ms
    int calls_after_resume = mock_sensor1_calls;
    
    SchedulerStop(&scheduler);
    pthread_join(scheduler_thread, NULL);
    
    TEST_ASSERT(calls_before_suspend > 0, "Sensor called before suspend");
    TEST_ASSERT_EQ(calls_before_suspend, calls_after_suspend, "Sensor not called during suspend");
    TEST_ASSERT(calls_after_resume > calls_after_suspend, "Sensor called after resume");
    
    SchedulerCleanup();
}

void test_scheduler_stats(void) {
    Scheduler scheduler;
    
    SchedulerInit(&scheduler, test_updateables);
    
    // This should not crash
    SchedulerGetStats();
    
    SchedulerCleanup();
}

void test_scheduler_invalid_hz(void) {
    Updateable invalid_sensor = {
        .name = "InvalidSensor",
        .hz = -1,  // Invalid frequency
        .update = mock_sensor1_update
        // Remove .data = NULL since it doesn't exist in your Updateable structure
    };
    
    Updateable* invalid_updateables[] = {
        &invalid_sensor,
        NULL
    };
    
    Scheduler scheduler;
    
    // Should handle invalid frequency gracefully
    SchedulerInit(&scheduler, invalid_updateables);
    
    TEST_ASSERT_EQ(0, scheduler.running, "Scheduler handles invalid frequency");
    
    SchedulerCleanup();
}

void scheduler_main(void) {
    TEST_SUITE_START("Scheduler");
    
    test_scheduler_init();
    test_scheduler_run_stop();
    test_scheduler_suspend_resume();
    test_scheduler_stats();
    test_scheduler_invalid_hz();
    
    TEST_SUITE_END();
}