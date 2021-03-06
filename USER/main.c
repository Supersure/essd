/**
 * proj @ essd_stdlib_freertos
 * 
 * @ breif
 * 		A stm32f1-based project with:
 * 			1. MPU6050 DMP attidude calculation;
 * 			2. 0.96' OLED human interface with GUISlim;
 * 			3. voltage measurement with 12-bit analog-digital converter;
 * 			4. input method with 4 keys;
 * 			5. communication with Android APPs via BlueTooth
 * 			6  communication with upper computers via serial port;
 * 			7. single mode & dual mode
 * 			8. remote control
 * 
 * @ dependency
 * 			stm32f10x standard lib
 * 			FreeRTOS kernel V9.0.0
 * 			GUISlim graphic library
 * 
 **/

#include "app.h"

extern long timestamp;

#define START_TASK_PRIO		1
#define MPU6050_TASK_PRIO	6
#define LED_TASK_PRIO		2
#define KEY_TASK_PRIO		5
#define ADC_TASK_PRIO		3
#define GUI_TASK_PRIO		5
#define TRANS_TASK_PRIO		4
#define RECEV_TASK_PRIO		4

#define START_STK_SIZE 		128
#define MPU6050_STK_SIZE	256
#define LED_STK_SIZE		160
#define KEY_STK_SIZE		160
#define ADC_STK_SIZE		128
#define GUI_STK_SIZE		160
#define TRANS_STK_SIZE		256
#define RECEV_STK_SIZE		160

TaskHandle_t StartTask_Handler;
TaskHandle_t MPU6050Task_Handler;
TaskHandle_t LedTask_Handler;
TaskHandle_t KeyTask_Handler;
TaskHandle_t AdConvertTask_Handler;
TaskHandle_t GuiTask_Handler;
TaskHandle_t TransferTask_Handler;
TaskHandle_t ReceiveTask_Handler;

static void start_task(void *pvParameters);
static void mpu6050_task(void *pvParameters);
static void led_task(void *pvParameters);
static void key_task(void *pvParameters);
static void adconvert_task(void *pvParameters);
static void gui_task(void *pvParameters);
static void transfer_task(void *pvParameters);
static void receive_task(void *pvParameters);

int main(void)
{
	bsp_init();

	GUI_SetFont(&GUI_FontHZ_SimSun_12);
	GUI_Clear();
	menu_init();
	
	xTaskCreate((TaskFunction_t )start_task,
	            (const char*    )"start_task",
	            (uint16_t       )START_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )START_TASK_PRIO,
	            (TaskHandle_t*  )&StartTask_Handler);


	vTaskStartScheduler();
	while(1);
}

/**
 * start_task
 * 
 * @brief:
 * 		create other tasks
 * 
 **/
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();

	xTaskCreate((TaskFunction_t )mpu6050_task,
	            (const char*    )"mpu6050_task",
	            (uint16_t       )MPU6050_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )MPU6050_TASK_PRIO,
	            (TaskHandle_t*  )&MPU6050Task_Handler);


	xTaskCreate((TaskFunction_t )led_task,
	            (const char*    )"led_task",
	            (uint16_t       )LED_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )LED_TASK_PRIO,
	            (TaskHandle_t*  )&LedTask_Handler);

	xTaskCreate((TaskFunction_t )key_task,
	            (const char*    )"key_task",
	            (uint16_t       )KEY_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )KEY_TASK_PRIO,
	            (TaskHandle_t*  )&KeyTask_Handler);

	xTaskCreate((TaskFunction_t )adconvert_task,
	            (const char*    )"adconvert_task",
	            (uint16_t       )ADC_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )ADC_TASK_PRIO,
	            (TaskHandle_t*  )&AdConvertTask_Handler);

	xTaskCreate((TaskFunction_t )gui_task,
	            (const char*    )"gui_task",
	            (uint16_t       )GUI_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )GUI_TASK_PRIO,
	            (TaskHandle_t*  )&GuiTask_Handler);
		    
	xTaskCreate((TaskFunction_t )transfer_task,
	            (const char*    )"transfer_task",
	            (uint16_t       )TRANS_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )TRANS_TASK_PRIO,
	            (TaskHandle_t*  )&TransferTask_Handler);
		    
	xTaskCreate((TaskFunction_t )receive_task,
	            (const char*    )"receive_task",
	            (uint16_t       )RECEV_STK_SIZE,
	            (void*          )NULL,
	            (UBaseType_t    )RECEV_TASK_PRIO,
	            (TaskHandle_t*  )&ReceiveTask_Handler);

	vTaskDelete(StartTask_Handler);
	taskEXIT_CRITICAL();
}

/**
 * mpu6050_task
 * 
 * @brief:
 * 		1. read raw data from MPU6050
 * 		2. calculate attitude
 * 
 **/
static void mpu6050_task(void *pvParameters)
{
	while(1) {

		/*get mpu6050 data unless in dual mode*/
		if(app.dual_mode != 2)
			MPU_getdata();
		vTaskDelay(50);
	}
}

static void led_task(void *pvParameters)
{
	uint8_t i = 1;
	uint8_t dir = 0;
	
	app.led_speed = 9;
	Led_Startup_1();

	while(1) {

		/*have entered system test mode page 1*/ 
		if(menu.page_index_depth == 1 && menu.page_index == 0 && menu.sub_page_index == 0) {
			if(app.led_speed > 0 && app.led_speed <= 9) {
				/*led flow logic*/
				if(i == 4)
					dir = 1;
				else if(i == 0)
					dir = 0;
				else;
				Led_Set_Status(i, led_off);
				if(dir == 0) {
					Led_Set_Status(i + 1, led_on);
					++i;
				}
				else {
					Led_Set_Status(i - 1, led_on);
					--i;
				}
				/*the delay time decides the speed*/
				delay_ms(1000 - app.led_speed * 100);
			}
		}
		/*have entered system test mode page 2*/ 
		else if(menu.page_index_depth == 1 && menu.page_index == 1) {
			Led_Set_Luminance(2, (fAX + 180) / 360 * 100);
			Led_Set_Luminance(3, (fAY + 180) / 360 * 100);
			Led_Set_Luminance(4, (fAZ + 180) / 360 * 100);
		}
		/*other modes*/ 
		else if(app.led_type)
			Led_Effect(app.led_type, app.led_speed, app.led_luminance * 10);
		else
			Led_Switch(app.led_luminance * 10, app.led);
		vTaskDelay(20);
	}
}

static void key_task(void *pvParameters)
{
	uint8_t key_sta = 0;
	while(1) {
		key_sta = Key_Scan();
		if(key_sta != 0) {
			/*when at main menu, use keys to move cursora and enter sub menu*/
			if(menu.flagIsMainMenu)  {
				switch(key_sta) {
				case 1:
					if(menu.page_index < 4)
						++menu.page_index;
					else;
					
					break;
				case 2:
					if(menu.page_index > 0)
						--menu.page_index;
					else;
					
					break;

				case 3:
					menu.flagIsMainMenu = 0;
					++menu.page_index_depth;
					break;
				case 4:
					menu.page_index = 0;
					menu.sub_page_index = 0;
					break;
				default:
					break;
				}
			} else {								/*when at sub menu, use keys for functions and sub menu interior switching */
				switch(key_sta) {
				/*key 1 pressed*/
				case 1:
					/*realize different functions when at different sub pages*/
					switch(menu.page_index) {
						case 0 :
							if(menu.sub_page_index < 3)
								++menu.sub_page_index;
							else;
							break;
						case 2 :
							if(menu.sub_page_index < 1)
								++menu.sub_page_index;
							else;
							break;
						
						case 3:
							if(app.pre_dual_mode < 2)
								++app.pre_dual_mode;
							break;
							
						case 4 :
							if(menu.sub_page_index < 1)
								++menu.sub_page_index;
							else;
							break;
							
						default :
							break;
						
					}
					break;
				/*key 2 pressed*/
				case 2:
					switch(menu.page_index) {
						case 0 :
							if(menu.sub_page_index > 0)
								--menu.sub_page_index;
							else;
							break;
							
						case 2 :
							if(menu.sub_page_index > 0)
								--menu.sub_page_index;
							else;
							break;
							
						case 3:
							if(app.pre_dual_mode > 0)
								--app.pre_dual_mode;
							break;
							
						case 4 :
							if(menu.sub_page_index > 0)
								--menu.sub_page_index;
							else;
							break;
						default :
							break;
						
					}
					break;
				/*key 3 pressed*/
				case 3:
					switch(menu.page_index) {
						case 2 :
							app.present_upload_interval = !app.present_upload_interval;
							break;
							
						case 3 :
							if(app.pre_dual_mode == 0) {
								app.present_upload_interval = 0;
								HC05_Init();
								app.dual_mode = 0;
							}
							if(app.pre_dual_mode == 1) {
								app.present_upload_interval = 0;
								HC05_Switch_2_Slave();
								app.dual_mode = 1;
							}
							else if(app.pre_dual_mode == 2) {
								app.present_upload_interval = 0;
								HC05_Switch_2_Master();
								app.dual_mode = 2;
							}
							break;
							
						case 4 :
							app.remote_switch = !app.remote_switch;
							break;
						default :
							break;
					}
					break;
				/*key 4 pressed*/
				case 4:
					menu.sub_page_index = 0;
					menu.flagIsMainMenu = 1;
					menu.page_index_depth = 0;
					break;
				/*key 1 long pressed*/
				case 5:
					switch(menu.page_index) {
						case 2 :
							if(app.present_upload_interval < 100)
								app.present_upload_interval += 10;
							else if(app.present_upload_interval < 1000)
								app.present_upload_interval += 100;
							else
								app.present_upload_interval = 10;
							break;
							
						case 3 :
							
							break;
							
						case 4 :
						
							break;
							
						default :
							break;
					}
					break;
				/*key 2 long pressed*/
				case 6:
					switch(menu.page_index) {
						case 2 :
							if(app.present_upload_frame < 2)
								++app.present_upload_frame;
							else
								app.present_upload_frame = 0;
							break;
							
						case 3 :
							
							break;
							
						case 4 :
							
							break;
							
						default :
							break;
					}
					break;
				default:
					break;
				}
			}
			/*when key is pressed, there may be sth happens to GUI, notify it for repainting*/
			xTaskNotifyGive(GuiTask_Handler);
		}
		vTaskDelay(20);
	}
}

/**
 * adconvert_task
 * 
 * @brief:
 * 		1. get ad value periodically
 * 		2. calculate relative voltage
 * 
 **/
static void adconvert_task(void *pvParameters)
{
	while(1) {
		if(app.dual_mode != 2)
			app.ad_value = Get_Adc_Average(1, 5);
		vTaskDelay(20);
	}
}

/**
 * transfer_task
 * 
 * @brief:
 * 		1. 	upload data to APP & upper computer
 * 		2. 	info->serial port->upper computer
 * 			info->Bluetooth->APP
 * 
 **/
static void transfer_task(void *pvParameters)
{
	while(1) {

		/* STATE = Bluetooth link status*/
		/* in case of yet connected with other BLE device, clear variables*/
		if(STATE == 0) {
			if(app.dual_name[0] != 0) {
				memset(app.dual_name, 0, sizeof(app.dual_name));
				app.handshake_count = 0;
			}
		}
		/*dual mode isn't set*/
		if(app.dual_mode == 0) {
			/*uploading data to upper computer via serial port perpectually*/
			while(app.present_upload_interval) {
				if(app.present_upload_frame == 0)
					app.upload_count += uploadBasicStatus();
				else if(app.present_upload_frame == 1)
					app.upload_count += uploadSamplingData(app.ad_value, ax, ay, az, gx, gy, gz);
				else if(app.present_upload_frame == 2)
					app.upload_count += uploadProcessedData(app.ad_value, fAX, fAY, fAZ);
				else;
				delay_ms(app.present_upload_interval);
			}
		}
		/*dual mode is set, give data to APP via Bluetooth*/
		else {
			if(EN == 0) {
				if(app.dual_name[0] == 0)
					app.upload_count += uploadBlueToothName((uint8_t *)student_id);
				else
					if(app.dual_mode == 1) 
						app.upload_count += uploadProcessedData(app.ad_value, fAX, fAY, fAZ);
			}
		}
		vTaskDelay(20);
	}
}

/**
 * receive_task
 * 
 * @brief:
 * 		1. 	receive data from upper computer via usart1
 * 		2. 	receive data from APP  via Bluetooth module (usart2)
 * 
 **/
static void receive_task(void *pvParameters)
{
	uint8_t parseRetVal = 0;
	while(1) {
		/*usart1 has received one packet*/
		if(USART_RX_STA & 0X8000) {
			/*parse packet*/
			parseRetVal = rcvDataParse(USART_RX_BUF);
			if(parseRetVal == 1) {
				app.recev_count += USART_RX_STA & 0X3FFF;
				app.present_recev_source = 1;
				USART_RX_STA = 0;
			}
			else if (parseRetVal == 2)	
				USART_RX_STA = 0;
		}
		/*usart2 has received one packet*/
		if(USART2_RX_STA & 0X8000) {
			/*parse packet*/
			parseRetVal = rcvDataParse(USART2_RX_BUF);
			if(parseRetVal == 1) {
				app.recev_count += USART2_RX_STA & 0X3FFF;
				app.present_recev_source = 2;
				USART2_RX_STA = 0;
			}
			else if (parseRetVal == 2)	
				USART2_RX_STA = 0;
		}
		vTaskDelay(20);
	}
}

/**
 * gui_task
 * 
 * @brief:
 * 		1. 	refresh screen to update GUI
 * 		2. 	construct dynamic elements & static elements
 **/
static void gui_task(void *pvParameters)
{
	menu_constructor_main_static();
	while(1) {
		/*main menu need no dynamic reconstructions*/
		if(menu.flagIsMainMenu) {
		}
		/*dynamic elements need timely reconstructions*/
		else {
			menu.present_constructor_dynamic = menu_constructors_dynamic[menu.page_index][menu.sub_page_index];
			menu.present_constructor_dynamic();
		}
		/*static elements are only reconstructed when message arrives*/
		if(ulTaskNotifyTake(pdTRUE, 0) > 0) {
			if(menu.flagIsMainMenu) {
				menu_constructor_main_static();
			} else {
				menu.present_constructor_static = menu_constructors_static[menu.page_index][menu.sub_page_index];
				menu.present_constructor_static();
				
			}
			
		}
		/*call update function to update screen*/
		GUI_Update();
		vTaskDelay(20);
	}
}




