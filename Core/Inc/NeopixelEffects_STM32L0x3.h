/*
 *  Created on: Sep 2, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.1
 *  File: NeopixelEffects_STM32L0x3.h
 *  Change history: N/A
 */

#ifndef INC_NEOPIXELEFFECTS_STM32L0X3_H_
#define INC_NEOPIXELEFFECTS_STM32L0X3_H_

#include "stdint.h"
#include "ClockDriver_STM32L0x3.h"
#include "BitBang_STM32L0x3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern uint8_t Bitbang_signal_source_matrix_32bit [][4];

//FUNCTION PROTOTYPES
void NeopixelPublish(void);
void NeopixelFillup_GRWB(uint8_t pixel_number, uint16_t fill_step_speed_in_ms);
void NeopixelDraindown_GRWB(uint8_t pixel_number, uint8_t strip_length, uint16_t drain_step_speed_in_ms);

#endif /* INC_NEOPIXELEFFECTS_STM32L0X3_H_ */
