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

#define NUM_TEST_FUNCTIONS 4

#define FN_MANAGER_STACK_SIZE 0x400

//X macros are magical! :)
//Order: function name, argument, stack size
#define TEST_FUNCTIONS \
    X(sanity,                       STACK_SIZE) \
    X(stack_reuse,                  STACK_SIZE) \
    X(say_hello,                    STACK_SIZE) \
    X(insanity,                     0x400)
//TODO more

#define NUM_PRIVILEGED_TESTS 2

//Making it to the manager is also a point!
#define NUM_TESTS (NUM_TEST_FUNCTIONS + NUM_PRIVILEGED_TESTS + 1)

#define INSANITY_LEVEL 20

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

static void insanity_helper(void*);

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

static volatile size_t insanity_counter = 0;

static const char* const LOGO = "\r\n\r\n\x1b[95m"
"             _                            _                 ____\r\n"
"  __ _ _   _| |_ ___   __ _ _ __ __ _  __| | ___ _ ____   _|___ \\\r\n"
" / _` | | | | __/ _ \\ / _` | '__/ _` |/ _` |/ _ \\ '__\\ \\ / / __) |\r\n"
"| (_| | |_| | || (_) | (_| | | | (_| | (_| |  __/ |   \\ V / / __/\r\n"
" \\__,_|\\__,_|\\__\\___/ \\__, |_|  \\__,_|\\__,_|\\___|_|    \\_/ |_____|\r\n"
"                      |___/\x1b[0m\r\n"
"\x1b[1m\"We're doing a sequel!\"\x1b[0m\r\n"
"\x1b[1mCopyright (C) 2024 \x1b[95mJohn Jekel\x1b[0m\x1b[1m and contributors\x1b[0m\r\n"
"\x1b[1mRepo: \x1b[96mhttps://github.com/JZJisawesome/autograderv2\x1b[0m\r\n\r\n";

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
    printf("\x1b[1mNote that a base level of functionality is required in order to run the autograder\r\n");
    printf("to completion without crashing. Even if you can't get that far right away,\r\n");
    printf("as you make progress you'll get further and further through the autograder\r\n");
    printf("code, which can be a great way to guage your progress in and of itself!\r\n");
    printf("Cheers and best of luck. Let's get into it! - \x1b[95mJZJ\x1b[0m\r\n\r\n");

    printf("\x1b[1mInitializing the kernel and doing some pre-osKernelStart() tests...\x1b[0m\r\n");

    int result = osTaskExit();
    if (result != RTX_ERR) {
        printf("\x1b[1m\x1b[91mosTaskExit() should return RTX_ERR when called from a privileged context!\x1b[0m\r\n");
    } else {
        printf("\x1b[1m\x1b[92mAwesome, you passed the first osTaskExit() test!\x1b[0m\r\n");
        ++num_passed;
    }

    osKernelStart();
    if (result != RTX_ERR) {
        printf("\x1b[1m\x1b[91mosKernelStart() should return RTX_ERR when called before the kernel was initialized!\x1b[0m\r\n");
    } else {
        printf("\x1b[1m\x1b[92mNice work on the pre-init osKernelStart() behavior!\x1b[0m\r\n");
        ++num_passed;
    }
    
    //TODO more tests pre-init

    osKernelInit();
    printf("\x1b[1mAlrighty, the kernel is initialized! Let's see how you're doing so far...\x1b[0m\r\n");
    print_score_so_far();

    //TODO more tests post-init

    TCB task;
    memset(&task, 0, sizeof(TCB));
    task.ptask      = test_function_manager;
    task.stack_size = FN_MANAGER_STACK_SIZE;
    osCreateTask(&task);

    //And off we go!
    printf("\r\n\x1b[1mOkay, I'm calling osKernelStart() now. This is a big step, don't get disheartened\r\n");
    printf("if it doesn't work on your first try, it certainly didn't for our group :)\r\n");
    printf("Before we leave though, here's your score so far:\x1b[0m\r\n");
    print_score_so_far();
    printf("\r\n\r\n");
    osKernelStart();
    assert(false && "\x1b[1m\x1b[92mosKernelStart() should never exit when called from a privileged context!\x1b[0m");
}

/* ------------------------------------------------------------------------------------------------
 * Static Function Implementations
 * --------------------------------------------------------------------------------------------- */

static void print_score_so_far(void) {
    double jekelscore_ratio = (double)num_passed / (double)NUM_TESTS;
    uint32_t jekelscore = (uint32_t)(jekelscore_ratio * 100);
    printf(
        "\x1b[1mYour \x1b[95mJekelScore\x1b[0m\x1b[1m is \x1b[96m%lu%%\x1b[0m\x1b[1m so far (\x1b[96m%d/%d\x1b[0m\x1b[1m passed)!\x1b[0m\r\n",
        jekelscore,
        num_passed,
        NUM_TESTS
    );
}

static void test_function_manager(void*) {
    printf("\x1b[1m\x1b[92mHaha, awesome you made it! This is being printed from a user task!\x1b[0m\r\n");
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
            "\x1b[1mRunning test function \x1b[96m#%u\x1b[0m\x1b[1m, \x1b[96m%s()\x1b[0m\x1b[1m, "
            "with a stack size of \x1b[96m%u\x1b[0m\x1b[1m bytes...\x1b[0m\r\n",
            ii + 1,
            test_functions[ii].name,
            test_functions[ii].stack_size
        );

        int result = osCreateTask(&task);
        assert((result == RTX_OK) && "\x1b[1m\x1b[91mFailed to create task!\x1b[0m");

        while (!function_complete) {
            osYield();
        }

        if (function_status) {
            printf(
                "\x1b[1m\x1b[92mTest function \x1b[96m#%u\x1b[92m, \x1b[96m%s()\x1b[92m, passed!\x1b[0m\r\n",
                ii,
                test_functions[ii].name
            );
            ++num_passed;
        } else {
            printf(
                "\x1b[1m\x1b[91mTest function \x1b[96m#%u\x1b[91m, \x1b[96m%s()\x1b[91m, failed!\x1b[0m\r\n",
                ii,
                test_functions[ii].name
            );
        }

        if ((ii + 1) != NUM_TEST_FUNCTIONS) {
            print_score_so_far();
        }
    }

    printf("\r\n\x1b[1m\x1b[92mYou made it to the end! :)\x1b[0m\r\n");

    print_score_so_far();

    if (num_passed == NUM_TESTS) {
        printf("\x1b[1m\x1b[92mYou passed all the tests with flying colours! Good stuff! :)\x1b[0m\r\n");
    } else {
        printf("\x1b[1m\x1b[93mYou didn't quite get them all, but don't give up! :)\x1b[0m\r\n");
    }

    printf("\x1b[1mHave an idea for a test? Submit a PR at \x1b[96mhttps://github.com/JZJisawesome/autograderv2\x1b[0m\x1b[1m!\x1b[0m\r\n");
    printf("\x1b[1mCheers and best of luck! - \x1b[95mJZJ\x1b[0m\r\n");

    printf("\r\n\x1b[1mI'll loop forever now, reset or reprogram the board to go again! :)\x1b[0m\r\n");
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

static void insanity_helper(void*) {
    printf("Hello there from TID %u!\r\n", getTID());
    ++insanity_counter;
}

static void insanity(void*) {
    printf("I have a bunch of friends who are going to say hello!\r\n");

    TCB task;
    memset(&task, 0, sizeof(TCB));
    task.ptask = insanity_helper;
    task.stack_size = STACK_SIZE;
    for (int ii = 0; ii < INSANITY_LEVEL; ++ii) {
        while (osCreateTask(&task) != RTX_OK) {
            osYield();
        }
    }

    while (insanity_counter < INSANITY_LEVEL) {
        osYield();
    }

    //TODO even more stress

    printf("And goodbye!\r\n");
    function_complete = true;
    function_status = true;
}
