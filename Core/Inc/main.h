/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
//we store the bitbang signal information in the struct below
typedef struct __output_signal_HandleTypeDef
{

  uint16_t            		TIM_freq_kHz;               /*!< frequency of the timer's clock
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   Note: this may not be the APB clock. There is usually a x2 on that, unless we are at max speed*/
  uint8_t            		TIM_clock_prescale;			/*!< local prescale of the TIM clock     */

  uint16_t            		PWM_freq_kHz;				/*!< frequency of the PWM signal. This will be the frequency of the output signal
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   Note: the PWM frequency will depend on the TIM freq, TIM prescale and the duty cycle RES*/

  uint8_t            		duty_cycle_RES;				/*!< the PWM duty cycle's resolution */

  uint16_t            		duty_cycle_LOW;				/*!< the PWM duty cycle that will define a LOW (0) state in the output signal - will be 0 for digital output */

  uint16_t            		duty_cycle_HIGH;			/*!< the PWM duty cycle that will define a HIGH (1) state in the output signal - will be 10 for digital output */

  uint8_t            		unique_period_no;			/*!< the number of unique periods within the signal - same as the number of elements in the input matrix*/

  uint8_t            		repeat_signal_no;				/*!< how many times the same signal should be repeated */

} output_signal_HandleTypeDef;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
//8-bit gamma control matrix
//Note: we add gamma control because the LED's (perceived) brightness is not actually linear with voltage
//Note: we move up and down within the gamma correction matrix to set the voltage values for the LED instead of giving the LED straight PWM values
static const uint8_t gamma_corr_8bit[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

typedef enum {
	No,
	Yes
} enum_Yes_No_Selector;


typedef enum {
	Blue_pushed,
	Other_pushed,
	None_pushed
} enum_buttons;



/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
