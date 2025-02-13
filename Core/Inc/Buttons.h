/*
 *  Created on: Sep 18, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.0
 *  File: Buttons.h
 *  Change history: N/A
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern enum_buttons buttons_push_state;

//FUNCTION PROTOTYPES
void Push_buttons_Init(void);
void Button_state_polling(void);

#endif /* INC_BUTTONS_H_ */
