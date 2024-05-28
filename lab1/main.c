/**
 * @file    main.c
 * @brief   Testcase for ECE 350, Lab 1
 * 
 * @copyright Copyright (C) 2024 John Jekel and contributors
 * See the LICENSE file at the root of the project for licensing info.
 * 
 * Replace your `main.cpp` file with this and you're off to the races!
 *
*/

/* ------------------------------------------------------------------------------------------------
 * Includes
 * --------------------------------------------------------------------------------------------- */

//These are your headers
#include "common.h"
#include "k_mem.h"
#include "k_task.h"

//These are headers that this testcase needs
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "main.h"

/* ------------------------------------------------------------------------------------------------
 * Constants/Defines
 * --------------------------------------------------------------------------------------------- */

#define NUM_TEST_FUNCTIONS 3

#define FN_MANAGER_STACK_SIZE 0x400

//X macros are magical! :)
//Order: function name, argument, stack size
#define TEST_FUNCTIONS \
    X(sanity,                       STACK_SIZE) \
    X(stack_reuse,                  STACK_SIZE) \
    X(say_hello,                    STACK_SIZE)
//TODO more

#define NUM_PRIVILEGED_TESTS 0

//Making it to the manager is also a point!
#define NUM_TESTS (NUM_TEST_FUNCTIONS + NUM_PRIVILEGED_TESTS + 1)

/* ------------------------------------------------------------------------------------------------
 * Type Declarations
 * --------------------------------------------------------------------------------------------- */

typedef struct {
    void              (*ptr)(void* args);
    const char* const   name;
    uint16_t            stack_size;
} test_function_info_s;

/* ------------------------------------------------------------------------------------------------
 * Static Function Declarations
 * --------------------------------------------------------------------------------------------- */

static void print_score_so_far(void);

static void test_function_manager(void*);

#define X(name, stack_size) static void name(void*);
TEST_FUNCTIONS
#undef X

/* ------------------------------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------------------------- */

static const test_function_info_s test_functions[NUM_TEST_FUNCTIONS] = {
    //These should set function_complete to true when they finish so we can move onto the next one
    //This synchronization mechanism works only if there's one test function running at once and
    //they only write true (while the test_function_manager reads it/writes false)
#define X(name, stack_size) {name, #name, stack_size},
    TEST_FUNCTIONS
#undef X
};

static volatile bool function_complete  = false;
static volatile bool function_status    = false;//False if failed, true if passed

static volatile size_t num_passed = 0;

static const char* const LOGO = ""
"             _                            _                 ____\r\n"
"  __ _ _   _| |_ ___   __ _ _ __ __ _  __| | ___ _ ____   _|___ \\\r\n"
" / _` | | | | __/ _ \\ / _` | '__/ _` |/ _` |/ _ \\ '__\\ \\ / / __) |\r\n"
"| (_| | |_| | || (_) | (_| | | | (_| | (_| |  __/ |   \\ V / / __/\r\n"
" \\__,_|\\__,_|\\__\\___/ \\__, |_|  \\__,_|\\__,_|\\___|_|    \\_/ |_____|\r\n"
"                      |___/\r\n"
"\"We're doing a sequel!\"\r\n"
"Copyright (C) 2024 John Jekel and contributors\r\n"
"Repo: https://github.com/JZJisawesome/autograderv2\r\n\r\n";

/* ------------------------------------------------------------------------------------------------
 * Function Implementations
 * --------------------------------------------------------------------------------------------- */

int main(void) {
    //Do things that the STM32CubeIDE does
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    //Logo!
    printf("%s", LOGO);
    printf("Note that a base level of functionality is required in order to run the autograder\r\n");
    printf("to completion without crashing. Even if you can't get that far right away,\r\n");
    printf("as you make progress you'll get further and further through the autograder\r\n");
    printf("code, which can be a great way to guage your progress in and of itself!\r\n");
    printf("Cheers and best of luck. Let's get into it! - JZJ\r\n\r\n");

    printf("Initializing the kernel and doing some pre-osKernelStart() tests...\r\n");

    //TODO test some things in main() itself
    osKernelInit();

    TCB task;
    memset(&task, 0, sizeof(TCB));
    task.ptask      = test_function_manager;
    task.stack_size = FN_MANAGER_STACK_SIZE;
    osCreateTask(&task);

    //And off we go!
    printf("Okay, I'm calling osKernelStart() now. This is a big step, don't get disheartened\r\n");
    printf("if it doesn't work on your first try, it certainly didn't for our group :)\r\n");
    osKernelStart();
    assert(false && "osKernelStart() should never exit when called from a privileged context!");
}

/* ------------------------------------------------------------------------------------------------
 * Static Function Implementations
 * --------------------------------------------------------------------------------------------- */

static void print_score_so_far(void) {
    double jekelscore_ratio = (double)num_passed / (double)NUM_TESTS;
    uint32_t jekelscore = (uint32_t)(jekelscore_ratio * 100);
    printf("Your JekelScore is %lu%% so far (%d/%d passed)!\r\n", jekelscore, num_passed, NUM_TESTS);
}

static void test_function_manager(void*) {
    printf("Haha, awesome you made it! This is being printed from a user task!\r\n");
    ++num_passed;
    print_score_so_far();

    for (size_t ii = 0; ii < NUM_TEST_FUNCTIONS; ++ii) {
        function_complete   = false;
        function_status     = false;

        TCB task;
        memset(&task, 0, sizeof(TCB));
        task.ptask      = test_functions[ii].ptr;
        task.stack_size = test_functions[ii].stack_size;
        
        printf(
            "Running test function #%u, %s(), with stack size %u...\r\n",
            ii + 1,
            test_functions[ii].name,
            test_functions[ii].stack_size
        );

        int result = osCreateTask(&task);
        assert((result == RTX_OK) && "Failed to create task!");

        while (!function_complete) {
            osYield();
        }

        if (function_status) {
            printf("Test function #%u, %s(), passed!\r\n", ii, test_functions[ii].name);
            ++num_passed;
        } else {
            printf("Test function #%u, %s(), failed!\r\n", ii, test_functions[ii].name);
        }

        if ((ii + 1) != NUM_TEST_FUNCTIONS) {
            print_score_so_far();
        }
    }

    printf("\r\nYou made it to the end! :)\r\n");

    print_score_so_far();

    if (num_passed == NUM_TESTS) {
        printf("You passed all the tests with flying colours! Good stuff! :)\r\n");
    } else {
        printf("You didn't quite get them all, but don't give up! :)\r\n");
    }

    printf("Have an idea for a test? Submit a PR at https://github.com/JZJisawesome/autograderv2!\r\n");
    printf("Cheers and best of luck! - JZJ\r\n");

    printf("I'll loop forever now, reset or reprogram the board to go again! :)\r\n");
    while (true);
}

static void sanity(void*) {
    //Do nothing!
    function_complete = true;
    function_status = true;
}

static void stack_reuse(void*) {
    //Same a sanity, but it should re-use sanity's stack
    //We can't really check this but at least it shouldn't crash :)
    function_complete = true;
    function_status = true;
}

static void say_hello(void*) {
    //Is the task environent robust enough to support calling printf()?
    //Almost certainly it is at this point if you're successfully running the test_manager_function(),
    //but hey, can you call printf from more than one task?
    printf("Hello World!\r\n");
    function_complete = true;
    function_status = true;
}
