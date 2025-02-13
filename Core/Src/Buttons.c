/*
 *  Created on: Sep 18, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 *  File: Buttons.c
 *  Change history: N/A
 */

#include "Buttons.h"

//1)
void Push_buttons_Init(void){
	/*
	 * Blue push button (PC13)
	 * Just a cable for other button on PC1/A4
	 *
	 * */

	//1)
	RCC->IOPENR |=	(1<<2);						//PORTC clocking
	GPIOC->MODER &= ~(1<<26);					//PC13 input
	GPIOC->MODER &= ~(1<<27);					//PC13 input
	GPIOC->PUPDR |= (1<<26);					//pull up
	GPIOC->PUPDR &= ~(1<<27);					//pull up

	GPIOC->MODER &= ~(1<<2);					//PC13 input
	GPIOC->MODER &= ~(1<<3);					//PC13 input
	GPIOC->PUPDR |= (1<<2);						//pull up
	GPIOC->PUPDR &= ~(1<<3);					//pull up

}

//2)
void Button_state_polling(void){
	/*
	 * we do a debouncing by checking the button states 2 ms apart
	 * if both are the same state, we have a verified button state
	 * blue button is on PC13
	 * the other button is to be on PC1
 	 */

	uint8_t blue_pushed_once = 0;
	uint8_t other_pushed_once = 0;

	if((GPIOC->IDR & (1<<13)) != (1<<13)){

		blue_pushed_once = 1;

	} else if ((GPIOC->IDR & (1<<1)) != (1<<1)){

		other_pushed_once = 1;

	} else {

		//do nothing

	}

	vTaskDelay(2 / portTICK_PERIOD_MS);

	if(((GPIOC->IDR & (1<<13)) != (1<<13)) & (blue_pushed_once == 1)){

		buttons_push_state = Blue_pushed;
		blue_pushed_once = 0;

	} else if (((GPIOC->IDR & (1<<1)) != (1<<1)) & (other_pushed_once == 1)){

		buttons_push_state = Other_pushed;
		other_pushed_once = 0;

	} else {

		//do nothing

	}


}
