/*
 *  Created on: Sep 16, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.0
 *  File: RTOS_threads.h
 *  Change history: N/A
 */

#ifndef INC_RTOS_THREADS_H_
#define INC_RTOS_THREADS_H_

#include "stddef.h"
#include "stdint.h"
#include "main.h"
#include "portmacro.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "Buttons.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

static uint16_t pixel_drain;
static uint16_t pixel_height;

static uint8_t water_mark_thread_1;
static uint8_t water_mark_thread_2;
static uint8_t water_mark_thread_3;
static uint32_t free_heap;
static uint32_t time_passed;
static char debug_char_buf[100];

//EXTERNAL VARIABLE
extern uint8_t pixels_in_strip;
extern uint8_t Bitbang_signal_source_matrix_32bit [][4];

extern TaskHandle_t task_1_ptr;
extern TaskHandle_t task_2_ptr;
extern TaskHandle_t task_3_ptr;
extern TaskHandle_t task_debug_ptr;
extern SemaphoreHandle_t data_source_mutex;

//FUNCTION PROTOTYPES

void Thread_Evaluate_Buttons(void *argument);
void Thread_PixelPublishTask(void *argument);
void Thread_Update_source_matrix(void *argument);
void Thread_Debug(void *argument);

#endif /* INC_RTOS_THREADS_H_ */
