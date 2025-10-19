#ifndef TEST_MODE
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#else
// Test mode includes
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#endif

#include "../../Inc/Scheduler/Scheduler.h"
#include "../../Inc/Scheduler/Task.h"

#ifdef TEST_MODE
// Test mode implementation using pthreads but keeping Task system

typedef struct {
    Updateable* updateable;
    timer_t timer_id;
    int active;
} ScheduledUpdateable;

// Work item structure - same as FreeRTOS version
typedef struct {
    Updateable* updateable;
    struct timespec timestamp;
} WorkItem;

typedef struct {
    WorkItem items[32];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} WorkQueue;

static ScheduledUpdateable scheduledTasks[MAX_SENSORS];
static int taskCount = 0;
static WorkQueue workQueue = {0};
static pthread_t workerThread;
static int schedulerRunning = 0;

// Queue operations for test mode
static void queueInit(WorkQueue* q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

static int queuePut(WorkQueue* q, const WorkItem* workItem) {
    pthread_mutex_lock(&q->mutex);
    
    if (q->count >= 32) {
        pthread_mutex_unlock(&q->mutex);
        return 0; // Queue full
    }
    
    q->items[q->rear] = *workItem;
    q->rear = (q->rear + 1) % 32;
    q->count++;
    
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

static int queueGet(WorkQueue* q, WorkItem* workItem) {
    pthread_mutex_lock(&q->mutex);
    
    while (q->count == 0 && schedulerRunning) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    if (!schedulerRunning) {
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    
    *workItem = q->items[q->front];
    q->front = (q->front + 1) % 32;
    q->count--;
    
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

// Worker thread that processes updateables using Task system
static void* workerTask(void* arg) {
    WorkItem workItem;
    
    while (schedulerRunning) {
        if (queueGet(&workQueue, &workItem)) {
            if (workItem.updateable != NULL) {
                // Record start time for performance monitoring
                struct timespec startTime;
                clock_gettime(CLOCK_MONOTONIC, &startTime);
                
                // Use the Task system properly - same as FreeRTOS version
                Task task;
                TaskInit(&task, workItem.updateable, workItem.updateable->hz);
                TaskExecute(&task);
                
                // Check execution time
                struct timespec endTime;
                clock_gettime(CLOCK_MONOTONIC, &endTime);
                double executionTime = (endTime.tv_sec - startTime.tv_sec) + 
                                     (endTime.tv_nsec - startTime.tv_nsec) / 1e9;
                double maxTime = (1.0 / workItem.updateable->hz) / 2.0; // 50% of period
                
                if (executionTime > maxTime) {
                    printf("WARNING: %s took %.3fms (max recommended: %.3fms)\n", 
                           workItem.updateable->name, executionTime * 1000, maxTime * 1000);
                }
            }
        }
    }
    return NULL;
}

// Timer callback for test mode - matches FreeRTOS behavior
static void timerCallback(union sigval sv) {
    Updateable* updateable = (Updateable*)sv.sival_ptr;
    
    if (updateable && schedulerRunning) {
        WorkItem workItem = {
            .updateable = updateable
        };
        clock_gettime(CLOCK_MONOTONIC, &workItem.timestamp);
        
        // Send to worker task - same as FreeRTOS version
        if (!queuePut(&workQueue, &workItem)) {
            printf("ERROR: Work queue full for %s - dropping task\n", updateable->name);
        }
    }
}

void SchedulerInit(Scheduler* scheduler, Updateable* updatableArray[]) {
    scheduler->running = 0;
    taskCount = 0;
    schedulerRunning = 1;
    
    // Create work queue for communicating between timer callbacks and worker task
    queueInit(&workQueue);
    
    // Create worker task to handle updateables
    if (pthread_create(&workerThread, NULL, workerTask, NULL) != 0) {
        printf("ERROR: Failed to create worker thread\n");
        return;
    }
    
    printf("Created scheduler worker task\n");
    
    // Create timers for each updateable - same logic as FreeRTOS version
    for (int i = 0; updatableArray[i] != NULL && i < MAX_SENSORS; i++) {
        Updateable* updateable = updatableArray[i];
        
        if (updateable->hz <= 0 || updateable->hz > MAX_HZ) {
            printf("Warning: Skipping %s - invalid frequency %d Hz\n", 
                   updateable->name, updateable->hz);
            continue;
        }
        
        // Create POSIX timer (equivalent to FreeRTOS software timer)
        struct sigevent sev;
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = timerCallback;
        sev.sigev_notify_attributes = NULL;
        sev.sigev_value.sival_ptr = updateable;
        
        timer_t timer_id;
        if (timer_create(CLOCK_MONOTONIC, &sev, &timer_id) == 0) {
            scheduledTasks[taskCount].updateable = updateable;
            scheduledTasks[taskCount].timer_id = timer_id;
            scheduledTasks[taskCount].active = 0;
            taskCount++;
            
            printf("Created timer: %s at %dHz\n", updateable->name, updateable->hz);
        } else {
            printf("ERROR: Failed to create timer for %s\n", updateable->name);
        }
    }
    
    printf("Scheduler initialized with %d timers and worker task\n", taskCount);
}

void SchedulerRun(Scheduler* scheduler) {
    scheduler->running = 1;
    
    // Start all timers - same logic as FreeRTOS version
    int started = 0;
    for (int i = 0; i < taskCount; i++) {
        double period = 1.0 / scheduledTasks[i].updateable->hz;
        
        struct itimerspec its;
        its.it_value.tv_sec = (time_t)period;
        its.it_value.tv_nsec = (long)((period - (time_t)period) * 1e9);
        its.it_interval = its.it_value;
        
        if (timer_settime(scheduledTasks[i].timer_id, 0, &its, NULL) == 0) {
            scheduledTasks[i].active = 1;
            started++;
        } else {
            printf("ERROR: Failed to start timer for %s\n", 
                   scheduledTasks[i].updateable->name);
        }
    }
    
    printf("Started %d/%d timers\n", started, taskCount);
    printf("Scheduler running in test mode - press Ctrl+C to stop\n");
    
    // In test mode, keep main thread alive (equivalent to vTaskStartScheduler())
    while (schedulerRunning) {
        usleep(100000); // 100ms
    }
}

void SchedulerStop(Scheduler* scheduler) {
    scheduler->running = 0;
    schedulerRunning = 0;
    
    // Stop all timers
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].active) {
            struct itimerspec its = {0};
            timer_settime(scheduledTasks[i].timer_id, 0, &its, NULL);
            scheduledTasks[i].active = 0;
        }
    }
    
    // Wake up worker thread and wait for it to finish
    pthread_cond_broadcast(&workQueue.cond);
    pthread_join(workerThread, NULL);
    
    printf("All timers stopped\n");
}

void SchedulerSuspendUpdateable(const char* name) {
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].updateable && 
            strcmp(scheduledTasks[i].updateable->name, name) == 0) {
            if (scheduledTasks[i].active) {
                struct itimerspec its = {0};
                timer_settime(scheduledTasks[i].timer_id, 0, &its, NULL);
                scheduledTasks[i].active = 0;
                printf("Suspended timer for %s\n", name);
                return;
            }
        }
    }
    printf("Warning: Could not find or suspend timer for %s\n", name);
}

void SchedulerResumeUpdateable(const char* name) {
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].updateable && 
            strcmp(scheduledTasks[i].updateable->name, name) == 0) {
            if (!scheduledTasks[i].active) {
                double period = 1.0 / scheduledTasks[i].updateable->hz;
                
                struct itimerspec its;
                its.it_value.tv_sec = (time_t)period;
                its.it_value.tv_nsec = (long)((period - (time_t)period) * 1e9);
                its.it_interval = its.it_value;
                
                if (timer_settime(scheduledTasks[i].timer_id, 0, &its, NULL) == 0) {
                    scheduledTasks[i].active = 1;
                    printf("Resumed timer for %s\n", name);
                    return;
                }
            }
        }
    }
    printf("Warning: Could not find or resume timer for %s\n", name);
}

void SchedulerGetStats(void) {
    pthread_mutex_lock(&workQueue.mutex);
    int queueCount = workQueue.count;
    pthread_mutex_unlock(&workQueue.mutex);
    
    printf("Scheduler Statistics (Test Mode):\n");
    printf("- Active timers: %d\n", taskCount);
    printf("- Work queue: %d items pending, %d spaces available\n", 
           queueCount, 32 - queueCount);
    
    if (queueCount > 24) { // 75% of 32
        printf("WARNING: Work queue is %d%% full - worker may be overloaded\n",
               (queueCount * 100) / 32);
    }
}

void SchedulerCleanup(void) {
    schedulerRunning = 0;
    
    // Stop all timers and delete them
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].active) {
            struct itimerspec its = {0};
            timer_settime(scheduledTasks[i].timer_id, 0, &its, NULL);
        }
        timer_delete(scheduledTasks[i].timer_id);
    }
    
    // Clean up worker thread
    pthread_cond_broadcast(&workQueue.cond);
    pthread_join(workerThread, NULL);
    
    // Clean up synchronization objects
    pthread_mutex_destroy(&workQueue.mutex);
    pthread_cond_destroy(&workQueue.cond);
    
    taskCount = 0;
    printf("Scheduler cleaned up\n");
}

#else
// Original FreeRTOS implementation...

typedef struct {
    Updateable* updateable;
    TimerHandle_t timer;
} ScheduledUpdateable;

// Work item for the worker task
typedef struct {
    Updateable* updateable;
    TickType_t timestamp;
} WorkItem;

static ScheduledUpdateable scheduledTasks[MAX_SENSORS];
static int taskCount = 0;

// Worker task handles
static TaskHandle_t workerTaskHandle = NULL;
static QueueHandle_t workQueue = NULL;

#define WORK_QUEUE_SIZE 32
#define WORKER_STACK_SIZE 1024
#define WORKER_PRIORITY (tskIDLE_PRIORITY + 2)

// Worker task that processes updateables
static void workerTask(void* pvParameters) {
    WorkItem workItem;
    
    while (1) {
        // Wait for work items from timer callbacks
        if (xQueueReceive(workQueue, &workItem, portMAX_DELAY) == pdTRUE) {
            if (workItem.updateable != NULL) {
                // Record start time for performance monitoring
                TickType_t startTime = xTaskGetTickCount();
                
                // Execute the updateable using Task system
                Task task;
                TaskInit(&task, workItem.updateable, workItem.updateable->hz);
                TaskExecute(&task);
                
                // Check execution time
                TickType_t executionTime = xTaskGetTickCount() - startTime;
                TickType_t maxTime = pdMS_TO_TICKS(1000 / workItem.updateable->hz) / 2; // 50% of period
                
                if (executionTime > maxTime) {
                    printf("WARNING: %s took %lu ticks (max recommended: %lu)\n", 
                           workItem.updateable->name, executionTime, maxTime);
                }
            }
        }
    }
}

// Fast timer callback - just queues work item
static void updateableTimerCallback(TimerHandle_t xTimer) {
    Updateable* updateable = (Updateable*)pvTimerGetTimerID(xTimer);
    
    if (updateable != NULL && workQueue != NULL) {
        WorkItem workItem = {
            .updateable = updateable,
            .timestamp = xTaskGetTickCount()
        };
        
        // Send to worker task (non-blocking from timer context)
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xQueueSendFromISR(workQueue, &workItem, &xHigherPriorityTaskWoken) != pdTRUE) {
            // Queue full - this indicates the worker task is overloaded
            printf("ERROR: Work queue full for %s - dropping task\n", updateable->name);
        }
        
        // Yield if higher priority task was woken
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void SchedulerInit(Scheduler* scheduler, Updateable* updatableArray[]) {
    scheduler->running = 0;
    taskCount = 0;

    // Create work queue for communicating between timer callbacks and worker task
    workQueue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(WorkItem));
    if (workQueue == NULL) {
        printf("ERROR: Failed to create work queue\n");
        return;
    }

    // Create worker task to handle updateables
    if (xTaskCreate(workerTask, "SchedulerWorker", WORKER_STACK_SIZE, NULL, 
                    WORKER_PRIORITY, &workerTaskHandle) != pdPASS) {
        printf("ERROR: Failed to create worker task\n");
        return;
    }

    printf("Created scheduler worker task\n");

    // Create timers for each updateable
    for (int i = 0; updatableArray[i] != NULL && i < MAX_SENSORS; i++) {
        Updateable* updateable = updatableArray[i];
        
        if (updateable->hz <= 0 || updateable->hz > MAX_HZ) {
            printf("Warning: Skipping %s - invalid frequency %d Hz\n", 
                   updateable->name, updateable->hz);
            continue;
        }

        // Calculate period in FreeRTOS ticks
        TickType_t period = pdMS_TO_TICKS(1000 / updateable->hz);
        if (period == 0) period = 1; // Minimum 1 tick for 1kHz tasks
        
        // Create timer name
        char timerName[32];
        snprintf(timerName, sizeof(timerName), "T_%s", updateable->name);
        
        // Create FreeRTOS software timer
        TimerHandle_t timer = xTimerCreate(
            timerName,                     // Timer name
            period,                        // Period in ticks
            pdTRUE,                       // Auto-reload (periodic)
            updateable,                   // Timer ID
            updateableTimerCallback       // Callback function
        );
        
        if (timer != NULL) {
            scheduledTasks[taskCount].updateable = updateable;
            scheduledTasks[taskCount].timer = timer;
            taskCount++;
            
            printf("Created timer: %s at %dHz (period: %lu ticks)\n", 
                   updateable->name, updateable->hz, period);
        } else {
            printf("ERROR: Failed to create timer for %s\n", updateable->name);
        }
    }
    
    printf("Scheduler initialized with %d timers and worker task\n", taskCount);
}

void SchedulerRun(Scheduler* scheduler) {
    scheduler->running = 1;
    
    // Start all timers
    int started = 0;
    for (int i = 0; i < taskCount; i++) {
        if (xTimerStart(scheduledTasks[i].timer, 0) == pdPASS) {
            started++;
        } else {
            printf("ERROR: Failed to start timer for %s\n", 
                   scheduledTasks[i].updateable->name);
        }
    }
    
    printf("Started %d/%d timers\n", started, taskCount);
    
    // Start FreeRTOS scheduler
    vTaskStartScheduler();
}

void SchedulerStop(Scheduler* scheduler) {
    scheduler->running = 0;
    
    // Stop all timers
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].timer != NULL) {
            xTimerStop(scheduledTasks[i].timer, portMAX_DELAY);
        }
    }
    
    printf("All timers stopped\n");
}

// Utility functions
void SchedulerSuspendUpdateable(const char* name) {
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].updateable && 
            strcmp(scheduledTasks[i].updateable->name, name) == 0) {
            if (xTimerStop(scheduledTasks[i].timer, 0) == pdPASS) {
                printf("Suspended timer for %s\n", name);
                return;
            }
        }
    }
    printf("Warning: Could not find or suspend timer for %s\n", name);
}

void SchedulerResumeUpdateable(const char* name) {
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].updateable && 
            strcmp(scheduledTasks[i].updateable->name, name) == 0) {
            if (xTimerStart(scheduledTasks[i].timer, 0) == pdPASS) {
                printf("Resumed timer for %s\n", name);
                return;
            }
        }
    }
    printf("Warning: Could not find or resume timer for %s\n", name);
}

void SchedulerGetStats(void) {
    UBaseType_t queueLength = uxQueueMessagesWaiting(workQueue);
    UBaseType_t queueSpaces = uxQueueSpacesAvailable(workQueue);
    
    printf("Scheduler Statistics:\n");
    printf("- Active timers: %d\n", taskCount);
    printf("- Work queue: %lu items pending, %lu spaces available\n", 
           queueLength, queueSpaces);
    
    if (workerTaskHandle != NULL) {
        TaskStatus_t taskStatus;
        vTaskGetInfo(workerTaskHandle, &taskStatus, pdTRUE, eInvalid);
        printf("- Worker task stack high water mark: %u words\n", 
               taskStatus.usStackHighWaterMark);
    }
    
    if (queueLength > (WORK_QUEUE_SIZE * 3 / 4)) {
        printf("WARNING: Work queue is %lu%% full - worker may be overloaded\n",
               (queueLength * 100) / WORK_QUEUE_SIZE);
    }
}

// Cleanup function for proper shutdown
void SchedulerCleanup(void) {
    // Delete worker task
    if (workerTaskHandle != NULL) {
        vTaskDelete(workerTaskHandle);
        workerTaskHandle = NULL;
    }
    
    // Delete work queue
    if (workQueue != NULL) {
        vQueueDelete(workQueue);
        workQueue = NULL;
    }
    
    // Delete all timers
    for (int i = 0; i < taskCount; i++) {
        if (scheduledTasks[i].timer != NULL) {
            xTimerDelete(scheduledTasks[i].timer, portMAX_DELAY);
            scheduledTasks[i].timer = NULL;
        }
    }
    
    taskCount = 0;
    printf("Scheduler cleaned up\n");
}

#endif