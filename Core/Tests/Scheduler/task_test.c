#include "../Test/test.h"
#include "../../Inc/Scheduler/Task.h"
#include "../../Inc/Utils/Updateable.h"

static int test_update_calls = 0;
static void* test_update_data = NULL;

static void test_update_function(void* data) {
    test_update_calls++;
    test_update_data = data;
}

void test_task_init(void) {
    Updateable updateable = {
        .name = "TestUpdateable",
        .hz = 100,
        .update = test_update_function,
        .data = (void*)0x12345678
    };
    
    Task task;
    TaskInit(&task, &updateable, 50);
    
    TEST_ASSERT_STR_EQ("TestUpdateable", task.updateable->name, "Task name set correctly");
    TEST_ASSERT_EQ(50, task.hz, "Task frequency set correctly");
    TEST_ASSERT_NOT_NULL(task.updateable, "Task updateable not null");
}

void test_task_execute(void) {
    test_update_calls = 0;
    test_update_data = NULL;
    
    Updateable updateable = {
        .name = "TestUpdateable",
        .hz = 100,
        .update = test_update_function,
        .data = (void*)0xDEADBEEF
    };
    
    Task task;
    TaskInit(&task, &updateable, 100);
    TaskExecute(&task);
    
    TEST_ASSERT_EQ(1, test_update_calls, "Update function called once");
    TEST_ASSERT_EQ((void*)0xDEADBEEF, test_update_data, "Update data passed correctly");
}

void test_task_null_updateable(void) {
    Task task;
    TaskInit(&task, NULL, 100);
    
    // Should handle null updateable gracefully
    TaskExecute(&task);
    
    TEST_ASSERT(true, "Task handles null updateable gracefully");
}

void test_task_null_update_function(void) {
    test_update_calls = 0;
    
    Updateable updateable = {
        .name = "TestUpdateable",
        .hz = 100,
        .update = NULL,  // Null update function
        .data = NULL
    };
    
    Task task;
    TaskInit(&task, &updateable, 100);
    TaskExecute(&task);
    
    // Should not crash
    TEST_ASSERT(true, "Task handles null update function gracefully");
}

void task_main(void) {
    TEST_SUITE_START("Task");
    
    test_task_init();
    test_task_execute();
    test_task_null_updateable();
    test_task_null_update_function();
    
    TEST_SUITE_END();
}