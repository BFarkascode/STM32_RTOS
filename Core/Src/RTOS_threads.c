/*
 *  Created on: Sep 16, 2024
 *  Author: BalazsFarkas
 *  Project: STM32_bareTOS
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 *  File: RTOS_threads.c
 *  Change history: N/A
 */


#include "RTOS_threads.h"

//1) Buttons evaluation thread
void Thread_Evaluate_Buttons(void *argument)
{
  /* USER CODE BEGIN Evaluate_ANO */
  /* Infinite loop */
  for(;;)
  {
	Button_state_polling();
	vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  //exit task if necessary
  vTaskDelete(NULL);

  /* USER CODE END Evaluate_ANO */
}


//2)Strip publishing thread
void Thread_PixelPublishTask(void *argument)
{
  for(;;)
  {
	  xSemaphoreTake(data_source_mutex, portMAX_DELAY);									//we wait until the mutex is available
  	  	  	  	  	  	  																//Note 1: this section is necessary to avoid the publish of the source matrix while we are updating it
  	  	  	  	  	  	  																//Note 2: the actual chance of overlap is minimal due to how fast we execute the code
  	  	  	  	  	  	  																//Note 3: mutex should be protected against priority inversion by implementing priority inheritance
	  NeopixelPublish();
	  vTaskDelay(1 / portTICK_PERIOD_MS);												//minimum delay we can add
	  	  	  	  	  	  	  	  	  	  	  	  	  									//Note: we need 80 us to signal the end of the strip
	  xSemaphoreGive(data_source_mutex);
	  vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  //exit task if necessary
  vTaskDelete(NULL);
}


//3)Update source matrix thread
void Thread_Update_source_matrix(void *argument)
{
  /* USER CODE BEGIN Update_source_matrix */
  /* Infinite loop */
  for(;;)
  {
	  switch (buttons_push_state) {

	  case Blue_pushed:

		  if(pixel_height == 0) {

			  Bitbang_signal_source_matrix_32bit [0][0] = 0;							//then we copy the 1st pixel to the 8th one
			  Bitbang_signal_source_matrix_32bit [0][1] = 251;
			  Bitbang_signal_source_matrix_32bit [0][2] = 0;

		  } else {

			  if(pixel_height % pixels_in_strip == 0){

				  Bitbang_signal_source_matrix_32bit [0][0] = Bitbang_signal_source_matrix_32bit [0][0] + 7;											//then we copy the 1st pixel to the 8th one
				  Bitbang_signal_source_matrix_32bit [0][1] = Bitbang_signal_source_matrix_32bit [0][1] - 14;
				  Bitbang_signal_source_matrix_32bit [0][2] = Bitbang_signal_source_matrix_32bit [0][2] + 21;

			  } else {

				  xSemaphoreTake(data_source_mutex, portMAX_DELAY);						//we wait until the mutex is available

				  NeopixelFillup_GRWB((pixel_height%pixels_in_strip) + 1, 0);

				  xSemaphoreGive(data_source_mutex);

				  buttons_push_state = None_pushed;

			  }

		  }

		  pixel_height++;


		  if((pixel_height / pixels_in_strip) == 0){

			  pixel_drain = pixel_height;

		  } else {

			  pixel_drain = pixels_in_strip;

		  }

		  break;

	  case Other_pushed :

		  if(pixel_drain == 0){

			  //do nothing
			  buttons_push_state = None_pushed;

		  } else {

			  pixel_drain--;

			  xSemaphoreTake(data_source_mutex, portMAX_DELAY);

			  NeopixelDraindown_GRWB(pixel_drain, pixels_in_strip, 0);

			  xSemaphoreGive(data_source_mutex);

			  buttons_push_state = None_pushed;

			  pixel_height = pixel_drain;

		  }

		  break;

	  case None_pushed :

		  //do nothing
		  buttons_push_state = None_pushed;

		  break;

	  default:

		  break;

	  }

	  vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  //exit task if necessary
  vTaskDelete(NULL);

  /* USER CODE END Update_source_matrix */
}


//4)Debug thread
void Thread_Debug(void *argument)
{
  for(;;)
  {
	  water_mark_thread_1 = uxTaskGetStackHighWaterMark(task_1_ptr);
	  water_mark_thread_2 = uxTaskGetStackHighWaterMark(task_2_ptr);
	  water_mark_thread_3 = uxTaskGetStackHighWaterMark(task_3_ptr);
	  free_heap = xPortGetFreeHeapSize();
	  time_passed = xTaskGetTickCount();
	  vTaskGetRunTimeStats(debug_char_buf);
	  vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  //exit task if necessary
  vTaskDelete(NULL);
}
