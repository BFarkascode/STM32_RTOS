/*
 *  Created on: Aug 30, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 *  File: BitBang_STM32L0x3.c
 *  Change history: N/A
 */

#include "BitBang_STM32L0x3.h"

//1) Setup for PWM using TIM2 and channel 1.
void TIM2_Config_for_CH1_PWM_DMA(void) {
	/**
	 * This part is just to enable the TIM2 CH1 to generate a PWM signal.
	 * We also enable the DMA to be engaged by the TIM2.
	 *
	 **/

	//1)Enable PORT clocking in the RCC, set MODER and OSPEEDR - PA5 LED2

	RCC->IOPENR |=	(1<<0);																//PORTA clocking - PA5 is LED2
	GPIOA->MODER &= ~(1<<10);															//alternate function for PA5
	GPIOA->MODER |= (1<<11);															//alternate function for PA5
	GPIOA->AFR[0] |= (5<<20);															//PA5 AF5 setup

	//2)Set TIM2_CH1
	RCC->APB1ENR |= (1<<0);																//enable TIM2 clocking
	TIM2->PSC = output_signal.TIM_clock_prescale - 1;									//we want a PWM running at 800 kHz
	TIM2->ARR = output_signal.duty_cycle_RES - 1;										//we set the resolution 0f the PWM - how far shall we count before we go back to 0

	//3)Adjust PWM
	TIM2->CCMR1 |= (1<<3);																//we activate the PWM functionality by output compare preload enabled
	TIM2->CCMR1 |= (6<<4);																//PWM mode 1 selected. CH1 will be "on" as long as CNT is lower than CCR1
	TIM2->CCR1 = 0;																		//we set an initial duty cycle of 0

	//4)Enable everything
	TIM2->CR1 |= (1<<7);																//ARP enabled (ARR will be buffered)
	TIM2->DIER |= (1<<9);																//DMA enabled on CH1 output compare

	//5)Update timer registers
	TIM2->EGR |= (1<<0);																//we update TIM2 with the new parameters. This generates an update event.

	while(!(TIM2->SR & (1<<0)));														//wait for the register update flag - UIF update interrupt flag
																						//it will go LOW - and thus, allow the code to pass - when the EGR-controlled TIM2 update has occurMSB_minus1_byte successfully and the registers are updated
																						//we have an update event at the end of each overflow - count up to - or underflow - count down to - moment. Count limit is defined by the ARR.
}


//2)We set up the basic driver for the DMA
void DMA_Init_for_TIM2_CH1(void){
	/*
	 * Initialize DMA form TIM2 CH1
	 */

	//1)
	RCC->AHBENR |= (1<<0);																//enable DMA clocking
																						//clocking is directly through the AHB

	//2)
	DMA1_CSELR->CSELR |= (1<<19);														//DMA1_Channel5: will be requested at 4'b1000 for TIM2_CH1
}

//3)We set up a channel for the TIM2 CH1
void Bitbang_TIM2CH1DMA1CH5_Config_32bit(void){
	/*
	 * Configure the DMA channel for TIM2 CH1
	 */

	//1)
	DMA1_Channel5->CCR &= ~(1<<0);														//we disable the DMA channel

	//2)
	DMA1_Channel5->CCR |= (1<<1);														//we enable the IRQ on the transfer complete
	DMA1_Channel5->CCR |= (1<<2);														//we enable the IRQ on the half transfer
	DMA1_Channel5->CCR |= (1<<3);														//we enable the error interrupt within the DMA channel
	DMA1_Channel5->CCR |= (1<<4);														//we read form memory to peripheral
	if((output_signal.repeat_signal_no - 1) == 0) {

		DMA1_Channel5->CCR &= ~(1<<5);													//circular mode is off

	} else {

		DMA1_Channel5->CCR |= (1<<5);													//circular mode is on

	}																					//peripheral increment is not used

	DMA1_Channel5->CCR |= (1<<7);														//memory increment is is used
	DMA1_Channel5->CCR &= ~(1<<8);														//peri side data length is 32 bits
	DMA1_Channel5->CCR |= (1<<9);														//peri side data length is 32 bits

	DMA1_Channel5->CCR &= ~(3<<10);														//mem side data length is 8 bits
																						//priority level is kept at LOW
																						//mem-to-mem is not used
	//3)
	DMA1_Channel5->CPAR = (uint32_t) (&(TIM2->CCR1));									//we want the data to be written to the TIM2's CCR1 register

	DMA1_Channel5->CMAR = (uint32_t) &Bitbang_PWM_signal_val_32bit_buf;					//this is the address (!) of the memory buffer we want to funnel data into

	//4)
	DMA1_Channel5->CNDTR = output_signal.unique_period_no;								//we want to have an element burst of "transfer_width"
																						//Note: circular mode will automatically reset this register

}


//4)We define what should happen upon DMA IRQ
void DMA1_Channel4_5_6_7_IRQHandler (void){

	//1)
	if ((DMA1->ISR & (1<<17)) == (1<<17)) {												//if we had the full transmission triggered on channel 5

		//do nothing
		if(repeat_cycle_no == (output_signal.repeat_signal_no - 1)){

			//turning off the system is rather complicated since we have everything buffered
			TIM2->CCR1 = 0;																//Note: this is to disable bleed over to the next round
			DMA1_Channel5->CCR &= ~(1<<0);												//we disable the DMA channel
																						//Note: this is to cut off the "tail" of the signal
			TIM2->CR1 &= ~(1<<0);														//timer counter disable bit
			TIM2->CCER &= ~(1<<0);														//we disable TIM2 CH1
			RCC->APB1ENR &= ~(1<<0);													//disable TIM2 clocking

			repeat_cycle_no = 0;
			Bitbang_signal_generated = 1;												//we set the bitbang signal generated flag
			TIM2->CNT = 0;
			Bitbang_signal_LSB_32bit(Bitbang_signal_source_matrix_32bit[1][0],Bitbang_signal_source_matrix_32bit[1][1],Bitbang_signal_source_matrix_32bit[1][2],Bitbang_signal_source_matrix_32bit[1][3]);		//preload the second value

		} else {

			repeat_cycle_no++;
			Bitbang_signal_LSB_32bit(Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no) * 2) + 1)][0],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no) * 2) + 1)][1],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no) * 2) + 1)][2],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no) * 2) + 1)][3]);

		}

	} else if ((DMA1->ISR & (1<<18)) == (1<<18)){										//if we had half transmission

		//here we load the even values (which will be odd numbered rows!!!) from the Bitbang_signal_source_matrix_32bit into the MSB position of the PWM data flow matrix
		if(repeat_cycle_no == (output_signal.repeat_signal_no - 1)) {

			Bitbang_signal_MSB_32bit(Bitbang_signal_source_matrix_32bit[0][0],Bitbang_signal_source_matrix_32bit[0][1],Bitbang_signal_source_matrix_32bit[0][2],Bitbang_signal_source_matrix_32bit[0][3]);	//we preload the first value
			DMA1_Channel5->CCR &= ~(1<<5);												//circular mode is off
																						//Note: for the last DMA execution, we must turn off the circularity, otherwise there is a "bleed over"
																						//the DMA will feed the TIM even as we are about to stop it

		} else {

			Bitbang_signal_MSB_32bit(Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no + 1) * 2))][0],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no + 1) * 2))][1],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no + 1) * 2))][2],Bitbang_signal_source_matrix_32bit[(((repeat_cycle_no + 1) * 2))][3]);

		}


	} else if ((DMA1->ISR & (1<<19)) == (1<<19)){										//if we had an error trigger on channel 5

		printf("DMA transmission error!");
		while(1);

	} else {

		//do nothing

	}

	DMA1->IFCR |= (1<<16);																//we remove all the interrupt flags from channel 5

}

//5)DMA IRQ priority
void DMA_TIM2_CH1_IRQPriorEnable(void) {
	/*
	 * We call the two special CMSIS functions to set up/enable the IRQ.
	 *
	 */
	NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 1);										//IRQ priority for channel 5
	NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);											//IRQ enable for channel 5
}


//6) Setup for a 4 byte long signal
void Bitbang_signal_MSB_32bit (uint8_t MSB_byte, uint8_t MSB_minus1_byte, uint8_t LSB_plus1_byte, uint8_t LSB_byte){

	/*
	 * This function loads the MSB row of the PWM ping-pong buffer (Bitbang_PWM_signal_val_32bit_buf).
	 */

	//we set up the PWM value matrix depending on the input
	if((LSB_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[0][31]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][31]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[0][30]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][30]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[0][29]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][29]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[0][28]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][28]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[0][27]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][27]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[0][26]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][26]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[0][25]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][25]= output_signal.duty_cycle_LOW;;

	}
	if((LSB_byte & (1 << 7)) == (1 << 7)){


		  Bitbang_PWM_signal_val_32bit_buf[0][24]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][24]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 0)) == (1 << 0)){


		  Bitbang_PWM_signal_val_32bit_buf[0][23]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][23]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[0][22]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][22]= output_signal.duty_cycle_LOW;;

	}
	if((LSB_plus1_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[0][21]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][21]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[0][20]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][20]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[0][19]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][19]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[0][18]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][18]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[0][17]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][17]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[0][16]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][16]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[0][15]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][15]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[0][14]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][14]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[0][13]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][13]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[0][12]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][12]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[0][11]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][11]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[0][10]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][10]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[0][9]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][9]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[0][8]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][8]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[0][7]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][7]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[0][6]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][6]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[0][5]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][5]= output_signal.duty_cycle_LOW;;

	}
	if((MSB_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[0][4]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][4]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[0][3]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][3]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[0][2]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][2]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[0][1]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][1]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[0][0]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[0][0]= output_signal.duty_cycle_LOW;;

	}

}

//7) Setup for a 4 byte long signal
void Bitbang_signal_LSB_32bit (uint8_t MSB_byte, uint8_t MSB_minus1_byte, uint8_t LSB_plus1_byte, uint8_t LSB_byte){

	/*
	 * This function loads the LSB row of the PWM ping-pong buffer (Bitbang_PWM_signal_val_32bit_buf).
	 */

	//we set up the PWM value matrix depending on the input
	if((LSB_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[1][31]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][31]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[1][30]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][30]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[1][29]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][29]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[1][28]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][28]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[1][27]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][27]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[1][26]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][26]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[1][25]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][25]= output_signal.duty_cycle_LOW;;

	}
	if((LSB_byte & (1 << 7)) == (1 << 7)){


		  Bitbang_PWM_signal_val_32bit_buf[1][24]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][24]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 0)) == (1 << 0)){


		  Bitbang_PWM_signal_val_32bit_buf[1][23]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][23]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[1][22]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][22]= output_signal.duty_cycle_LOW;;

	}
	if((LSB_plus1_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[1][21]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][21]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[1][20]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][20]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[1][19]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][19]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[1][18]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][18]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[1][17]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][17]= output_signal.duty_cycle_LOW;;

	}

	if((LSB_plus1_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[1][16]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][16]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[1][15]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][15]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[1][14]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][14]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[1][13]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][13]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[1][12]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][12]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[1][11]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][11]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[1][10]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][10]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[1][9]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][9]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_minus1_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[1][8]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][8]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 0)) == (1 << 0)){

		  Bitbang_PWM_signal_val_32bit_buf[1][7]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][7]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 1)) == (1 << 1)){

		  Bitbang_PWM_signal_val_32bit_buf[1][6]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][6]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 2)) == (1 << 2)){

		  Bitbang_PWM_signal_val_32bit_buf[1][5]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][5]= output_signal.duty_cycle_LOW;;

	}
	if((MSB_byte & (1 << 3)) == (1 << 3)){

		  Bitbang_PWM_signal_val_32bit_buf[1][4]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][4]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 4)) == (1 << 4)){

		  Bitbang_PWM_signal_val_32bit_buf[1][3]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][3]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 5)) == (1 << 5)){

		  Bitbang_PWM_signal_val_32bit_buf[1][2]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][2]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 6)) == (1 << 6)){

		  Bitbang_PWM_signal_val_32bit_buf[1][1]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][1]= output_signal.duty_cycle_LOW;;

	}

	if((MSB_byte & (1 << 7)) == (1 << 7)){

		  Bitbang_PWM_signal_val_32bit_buf[1][0]= output_signal.duty_cycle_HIGH;;

	} else {

		  Bitbang_PWM_signal_val_32bit_buf[1][0]= output_signal.duty_cycle_LOW;;

	}

}


//38) We restart the bitbang machine
void Bitbang_TIM2CH1DMA1CH5_Restart(void){
	/*
	 * Restart the bitbang machine
	 */

	  DMA1_Channel5->CCR &= ~(1<<0);												//we disable the DMA channel
	  	if((output_signal.repeat_signal_no - 1) == 0) {

	  		DMA1_Channel5->CCR &= ~(1<<5);											//circular mode is off

	  	} else {

	  		DMA1_Channel5->CCR |= (1<<5);											//circular mode is on

	  	}																			//peripheral increment is not used
	  	DMA1_Channel5->CNDTR = output_signal.unique_period_no;						//we want to have an element burst of "transfer_width"
	  	RCC->APB1ENR |= (1<<0);														//enable TIM2 clocking
	  	TIM2->CR1 |= (1<<0);														//timer counter enable bit
	  	TIM2->CCER |= (1<<0);														//we enable CH1
	  	DMA1_Channel5->CCR |= (1<<0);												//we enable the DMA channel

}

//9) Start signal generation
void Bitbang_GenerateStartMatrix_32bit(uint8_t bit_4, uint8_t bit_3, uint8_t bit_2, uint8_t bit_1, uint8_t pixel_number){

	/*
	 * This function generates a start state for the signal data matrix
	 * It also loads the first two values into the PWM ping-pong buffer
	 *
	 */

	  Bitbang_signal_source_matrix_32bit [0][0] = bit_4;							//then we copy the 1st pixel to the 8th one
	  Bitbang_signal_source_matrix_32bit [0][1] = bit_3;
	  Bitbang_signal_source_matrix_32bit [0][2] = bit_2;
	  Bitbang_signal_source_matrix_32bit [0][3] = bit_1;


	  Bitbang_signal_source_matrix_32bit [1][0] = bit_4;							//then we copy the 1st pixel to the 8th one
	  Bitbang_signal_source_matrix_32bit [1][1] = bit_3;
	  Bitbang_signal_source_matrix_32bit [1][2] = bit_2;
	  Bitbang_signal_source_matrix_32bit [1][3] = bit_1;

	  for(int i = 1; i < (pixel_number - 1); i++){

		  Bitbang_signal_source_matrix_32bit [i][0] = bit_4;						//then we copy the 1st pixel to the 8th one
		  Bitbang_signal_source_matrix_32bit [i][1] = bit_3;
		  Bitbang_signal_source_matrix_32bit [i][2] = bit_2;
		  Bitbang_signal_source_matrix_32bit [i][3] = bit_1;

	  }

	  //we need to pre-load the first two values we will be publishing so the ping-pong buffer will be full at the start
	  Bitbang_signal_MSB_32bit(Bitbang_signal_source_matrix_32bit[0][0],Bitbang_signal_source_matrix_32bit[0][1],Bitbang_signal_source_matrix_32bit[0][2],Bitbang_signal_source_matrix_32bit[0][3]);	//we generate a 4 byte signal
	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  									//Note: this will represent one GRBW pixel
	  Bitbang_signal_LSB_32bit(Bitbang_signal_source_matrix_32bit[1][0],Bitbang_signal_source_matrix_32bit[1][1],Bitbang_signal_source_matrix_32bit[1][2],Bitbang_signal_source_matrix_32bit[1][3]);  	//we load the second pixel as well to generate the ping-pong buffer

}

