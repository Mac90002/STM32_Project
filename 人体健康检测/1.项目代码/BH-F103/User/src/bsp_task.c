/*
#include "bsp_task.h"
#include "FreeRTOS.h"
#include <stdlib.h>
#include <math.h>
#include "task.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_SysTick.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_usart.h"
#include "bsp_mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "max30102.h"

TaskHandle_t start_task_handler;
void start_task(void *pvParameters);

TaskHandle_t task1_handler;
void task1(void *pvParameters);

TaskHandle_t task2_handler;
void task2(void *pvParameters);

static void Max30100_Task();
static void MPU6050_Task();



extern char disp_buff[200];
extern float pitch, roll, yaw, temp;
extern short ACC[3];
extern char ptr0[200];
extern char ptr10[200];

uint16_t SVM;
uint16_t flag;
uint16_t flag1;
uint16_t onlie_flag;


void OS_Task(void)
{

 xTaskCreate((TaskFunction_t)start_task,
             (char *)"start_task",
             (uint16_t)START_TASK_STACK_SIZE,
             (void *)NULL,
             (UBaseType_t)START_TASK_PRIO,
             (TaskHandle_t *)&start_task_handler);
 vTaskStartScheduler();
}

void start_task(void *pvParameters)
{	
 taskENTER_CRITICAL(); // 进入临界区
 xTaskCreate((TaskFunction_t)task1,
             (char *)"task1",
             (uint16_t)TASK1_STACK_SIZE,
             (void *)NULL,
             (UBaseType_t)TASK1_PRIO,
             (TaskHandle_t *)&task1_handler);

 xTaskCreate((TaskFunction_t)task2,
             (char *)"task2",
             (uint16_t)TASK2_STACK_SIZE,
             (void *)NULL,
             (UBaseType_t)TASK2_PRIO,
             (TaskHandle_t *)&task2_handler);						
 vTaskDelete(NULL);
	
 taskEXIT_CRITICAL(); // 退出临界区
}

void task1(void *pvParameters)
{
 while (1)
 {
		MPU6050_Task();
		
 }
}

void task2(void *pvParameters)
{
 while (1)
 {
		//Max30100_Task();
		vTaskDelay(10);
 }
}
		
static void MPU6050_Task()
{

	//mpu_dmp_get_data(&pitch, &roll, &yaw);
	MPU6050ReadAcc(ACC);
	SVM = sqrt((ACC[0] * ACC[0]) + (ACC[1] * ACC[1]) + (ACC[2] * ACC[2]));
	printf("SVM : %d\r\n", SVM);
	if (SVM<28000 & SVM> 10000)
	{
		flag--;
		if (flag == 0)
		{
			ILI9341_Clear(20, 20, 240, 20);
			ILI9341_DispString_EN(20, 20, "fall down : normal");
		}
	}
	else
	{
		ILI9341_Clear(20, 20, 240, 20);
		ILI9341_DispString_EN(20, 20, "fall down(SVM) : abnormal");
		flag = 250;
	}

//	if (roll > 50 | roll < -60)
//	{
//		ILI9341_Clear(0, 40, 240, 40);
//		ILI9341_DispString_EN(20, 40, "fall down(Euler Angle) : abnormal");
//		flag1 = 250;

////		sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":0}}\",0,0");
////		ESP8266_Cmd(ptr10, "OK", "NULL", 10);
//	}
//	else
//	{
//		flag1--;
//		if (flag1 == 0)
//		{
//			ILI9341_Clear(0, 40, 240, 40);
//			ILI9341_DispString_EN(20, 40, "fall down : normal");
//		}
//		sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":1}}\",0,0");
//		ESP8266_Cmd(ptr10, "OK", "NULL", 10);
	}
	//printf("pitch=%.2f   roll=%.2f   yaw=%.2f  \r\n", pitch, roll, yaw);




static void Max30100_Task()
{
	Gain_Hr_Spo();
}
		
  


*/
// wifi网卡 <-> 手机热点 <-> 电脑
// wifi 这个做 TCP client 连接我手机热点 手机热点就好比是个路由器？
// 然后我电脑也连接 手机热点 电脑端做 TCP server 
