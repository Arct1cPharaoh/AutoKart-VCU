#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Forward declare the main functions
void lut_main();
void wheel_speed_main();
void torque_control_main();
void apps_main();
void bms_main();
void scheduler_main();
void sensor_main();
void system_main();
void output_main();
void task_main();

// Defines a map from a string to a main function.
typedef struct {
    const char *name;
    void (*main)();
} map;

static map name_to_main[] = {
    {"apps", apps_main},
    {"bms", bms_main},
    {"lut", lut_main},
    {"output", output_main},
    {"scheduler", scheduler_main},
    {"sensor", sensor_main},
    {"system", system_main},
    {"task", task_main},
    {"torque_control", torque_control_main},
    {"wheel_speed", wheel_speed_main}
};
#define NUM_TESTS (sizeof(name_to_main) / sizeof(name_to_main[0]))

void run_all_tests() {
    printf("\n🚀 =====================================\n");
    printf("🚀 VEHICLE CONTROL UNIT - TEST SUITE\n");
    printf("🚀 =====================================\n");
    
    int passed = 0, total = NUM_TESTS;
    
    for (int n = 0; n < NUM_TESTS; ++n) {
        printf("\n📋 Running %s tests...\n", name_to_main[n].name);
        
        // Fork to isolate test failures
        pid_t pid = fork();
        if (pid == 0) {
            // Child process runs the test
            name_to_main[n].main();
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            // Parent waits for result
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf("📋 %s: ✅ PASSED\n", name_to_main[n].name);
                passed++;
            } else {
                printf("📋 %s: ❌ FAILED\n", name_to_main[n].name);
            }
        }
    }
    
    printf("\n🏁 =====================================\n");
    printf("🏁 FINAL RESULTS\n");
    printf("🏁 =====================================\n");
    printf("🏁 Passed: %d/%d tests\n", passed, total);
    
    if (passed == total) {
        printf("🏁 🎉 ALL TESTS PASSED! 🎉\n");
        printf("🏁 =====================================\n");
        exit(EXIT_SUCCESS);
    } else {
        printf("🏁 ❌ %d TESTS FAILED\n", total - passed);
        printf("🏁 =====================================\n");
        exit(EXIT_FAILURE);
    }
}

void run_test(const char *name) {
    for (int n = 0; n < NUM_TESTS; ++n) {
        map entry = name_to_main[n];
        if (strncmp(entry.name, name, strlen(entry.name)) == 0) {
            printf("🔍 Running %s test...\n", name);
            entry.main();
            printf("🔍 %s test completed successfully!\n", name);
            exit(EXIT_SUCCESS);
        }
    }
    
    fprintf(stderr, "❌ Test '%s' not found. Available tests:\n", name);
    for (int n = 0; n < NUM_TESTS; ++n) {
        fprintf(stderr, "   • %s\n", name_to_main[n].name);
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    bool all = false;
    bool verbose = false;

    int opt;
    while ((opt = getopt(argc, argv, "avh")) != -1) {
        switch (opt) {
        case 'a':
            all = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 'h':
            printf("Usage: %s [-a] [-v] [tests]\n", argv[0]);
            printf("Options:\n");
            printf("  -a        Run all tests\n");
            printf("  -v        Verbose output (show passed tests)\n");
            printf("  -h        Show this help\n");
            printf("Available tests:\n");
            for (int n = 0; n < NUM_TESTS; ++n) {
                printf("  • %s\n", name_to_main[n].name);
            }
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Usage: %s [-a] [-v] [tests]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (all) {
        run_all_tests();
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected name of test. Use -h for help.\n");
        exit(EXIT_FAILURE);
    }

    for (int n = optind; n < argc; ++n) {
        run_test(argv[n]);
    }

    return EXIT_SUCCESS;
}
