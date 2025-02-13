/*
 *  Created on: Aug 30, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.0
 *  File: BitBang_STM32L0x3.h
 *  Change history: N/A
 */

#ifndef INC_BITBANG_STM32L0X3_H_
#define INC_BITBANG_STM32L0X3_H_

#include "main.h"
#include "stdint.h"
#include "stm32l053xx.h"

//LOCAL CONSTANT

//LOCAL VARIABLE
static uint8_t repeat_cycle_no = 0;
static uint8_t Bitbang_PWM_signal_val_32bit_buf[2][32];

//EXTERNAL VARIABLE
extern output_signal_HandleTypeDef output_signal;
extern uint8_t Bitbang_signal_generated;
extern uint8_t Bitbang_signal_source_matrix_32bit[][4];

//FUNCTION PROTOTYPES
void TIM2_Config_for_CH1_PWM_DMA(void);
void DMA_Init_for_TIM2_CH1(void);
void DMA_TIM2_CH1_IRQPriorEnable(void);
void Bitbang_TIM2CH1DMA1CH5_Config_32bit(void);
void Bitbang_TIM2CH1DMA1CH5_Restart(void);
void Bitbang_signal_MSB_32bit (uint8_t MSB_byte, uint8_t MSB_minus1_byte, uint8_t LSB_plus1_byte, uint8_t LSB_byte);
void Bitbang_signal_LSB_32bit (uint8_t MSB_byte, uint8_t MSB_minus1_byte, uint8_t LSB_plus1_byte, uint8_t LSB_byte);
void Bitbang_GenerateStartMatrix_32bit(uint8_t bit_4, uint8_t bit_3, uint8_t bit_2, uint8_t bit_1, uint8_t pixel_number);

#endif /* INC_BITBANG_STM32L0X3_H_ */
