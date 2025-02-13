/*
 *  Created on: Sep 2, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.1
 *  File: NeopixelEffects_STM32L0x3.c
 *  Change history:
 *
 *  v.1.1
 *  	Removed unused functions
 *  	Removed delay function from the publish function to ensure reentry compliance (probably not necessary, but doesn't hurt)
 */

#include "NeopixelEffects_STM32L0x3.h"

//1)
void NeopixelPublish(void){

	/*
	 * This function publishes the data matrix onto the neopixel strip using the bitband machine
	 *
	 */

      Bitbang_TIM2CH1DMA1CH5_Restart();													//we shut everything off at the end of the strip
	  while(Bitbang_signal_generated == 0);
	  Bitbang_signal_generated = 0;

}

//2)
void NeopixelFillup_GRWB(uint8_t pixel_number, uint16_t fill_step_speed_in_ms){

	/*
	 * This function fills up the strip from bottom to top until a designated pixel with the colour on the first pixel.
	 *
	 */

	  for(int i = 1; i <= (pixel_number - 1); i++){

		Bitbang_signal_source_matrix_32bit[i][0] = Bitbang_signal_source_matrix_32bit[i-1][0];										//then we copy the 1st pixel to the 8th one
		Bitbang_signal_source_matrix_32bit[i][1] = Bitbang_signal_source_matrix_32bit[i-1][1];
		Bitbang_signal_source_matrix_32bit[i][2] = Bitbang_signal_source_matrix_32bit[i-1][2];
		Bitbang_signal_source_matrix_32bit[i][3] = Bitbang_signal_source_matrix_32bit[i-1][3];

	  }

	vTaskDelay(fill_step_speed_in_ms / portTICK_PERIOD_MS);

	//we need to pre-load the first two values we will be publishing so the ping-pong buffer will be full with the new matrix first two rows
	Bitbang_signal_MSB_32bit(Bitbang_signal_source_matrix_32bit[0][0],Bitbang_signal_source_matrix_32bit[0][1],Bitbang_signal_source_matrix_32bit[0][2],Bitbang_signal_source_matrix_32bit[0][3]);
	Bitbang_signal_LSB_32bit(Bitbang_signal_source_matrix_32bit[1][0],Bitbang_signal_source_matrix_32bit[1][1],Bitbang_signal_source_matrix_32bit[1][2],Bitbang_signal_source_matrix_32bit[1][3]);

}

//3)
void NeopixelDraindown_GRWB(uint8_t pixel_number, uint8_t strip_length, uint16_t drain_step_speed_in_ms){

	/*
	 * This function drains down the strip from top to bottom until a designated pixel using a blank colour.
	 *
	 */

	  for(int i = (strip_length - 1); i >= pixel_number; i--){

		Bitbang_signal_source_matrix_32bit[i][0] = 0;										//then we copy the 1st pixel to the 8th one
		Bitbang_signal_source_matrix_32bit[i][1] = 0;
		Bitbang_signal_source_matrix_32bit[i][2] = 0;
		Bitbang_signal_source_matrix_32bit[i][3] = 0;

	  }

	vTaskDelay(drain_step_speed_in_ms / portTICK_PERIOD_MS);

	//we need to pre-load the first two values we will be publishing so the ping-pong buffer will be full with the new matrix first two rows
	Bitbang_signal_MSB_32bit(Bitbang_signal_source_matrix_32bit[0][0],Bitbang_signal_source_matrix_32bit[0][1],Bitbang_signal_source_matrix_32bit[0][2],Bitbang_signal_source_matrix_32bit[0][3]);
	Bitbang_signal_LSB_32bit(Bitbang_signal_source_matrix_32bit[1][0],Bitbang_signal_source_matrix_32bit[1][1],Bitbang_signal_source_matrix_32bit[1][2],Bitbang_signal_source_matrix_32bit[1][3]);

}
