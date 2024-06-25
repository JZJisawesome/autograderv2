/**
 * @file    lab1_test4ish.c
 * @brief   One-off standalone test to more accurately replicate the real test4
 *
 * @copyright Copyright (C) 2024 John Jekel and contributors
 * See the LICENSE file at the root of the project for licensing info.
 *
*/

/* ------------------------------------------------------------------------------------------------
 * Includes
 * --------------------------------------------------------------------------------------------- */

//These are your headers
#include "common.h"
#include "k_mem.h"
#include "k_task.h"

//These are headers that we need
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

/* ------------------------------------------------------------------------------------------------
 * Static Function Declarations
 * --------------------------------------------------------------------------------------------- */

static void task1fn(void*);
static void task2fn(void*);
static void task3fn(void*);

/* ------------------------------------------------------------------------------------------------
 * Function Implementations
 * --------------------------------------------------------------------------------------------- */

int main(void) {
    //Do things that the STM32CubeIDE does
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    printf("NOTE! This isn't an autograder! You have to check the output for correctness!\r\n");
    osKernelInit();

    TCB task1;
    memset(&task1, 0, sizeof(TCB));
    task1.ptask = task1fn;
    task1.stack_size = 0x200;

    TCB task2;
    memset(&task2, 0, sizeof(TCB));
    task2.ptask = task2fn;
    task2.stack_size = 0x200;

    TCB task3;
    memset(&task3, 0, sizeof(TCB));
    task3.ptask = task3fn;
    task3.stack_size = 0x200;

    int result = osCreateTask(&task1);
    assert((result == RTX_OK) && "Failed to create task1!");
    result = osCreateTask(&task2);
    assert((result == RTX_OK) && "Failed to create task2!");
    result = osCreateTask(&task3);
    assert((result == RTX_OK) && "Failed to create task3!");

    osKernelStart();
    assert(false && "osKernelStart returned!");
}

/* ------------------------------------------------------------------------------------------------
 * Static Function Implementations (Test Functions)
 * --------------------------------------------------------------------------------------------- */

static void task1fn(void*) {
    for (int i = 0; i < 10; i++) {
        printf("task-1\r\n");
        osYield();
    }
    osTaskExit();
}

static void task2fn(void*) {
    for (int i = 0; i < 5; i++) {
        printf("task-2\r\n");
        osYield();
    }
    osTaskExit();
}

static void task3fn(void*) {
    for (int i = 0; i < 10; i++) {
        printf("task-3\r\n");
        osYield();
    }
    osTaskExit();
}
