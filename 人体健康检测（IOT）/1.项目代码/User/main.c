#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "bsp_usart.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_SysTick.h"
#include "bsp_task.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "bsp_esp8266.h"
#include "core_delay.h"
#include "max30102.h"
#include <math.h>

void MPU6050_Task(void);
void ESP8266_Task(void);

char disp_buff[200];
float pitch, roll, yaw, temp;
short ACC[3];
char ptr0[200];
char ptr10[200];

uint16_t SVM = 0;
uint16_t flag = 1;
uint16_t flag1 = 1;
uint16_t onlie_flag = 0;

int main()
{
	SysTick_Init();
	LED_GPIO_Config();
	Key_GPIO_Config();
	USART_Config();
	ILI9341_Config();
	CPU_TS_TmrInit();
	ESP8266_Init();
	ILI9341_Clear(0, 0, 240, 320);
	macESP8266_CH_ENABLE();
	ESP8266_MQTT_Config();
	max3012_init();
	MPU6050_Init();
	/*用OS，有速度不匹配问题*/
	// OS_Task();

	while (1)
	{
		/*MPU6050*/
		onlie_flag++;
		MPU6050ReadAcc(ACC);
		SVM = sqrt((ACC[0] * ACC[0]) + (ACC[1] * ACC[1]) + (ACC[2] * ACC[2]));
		printf("SVM : %d\r\n", SVM);
		if (SVM<19000 & SVM> 13000)
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
			sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":0}}\",0,0");
			ESP8266_Cmd(ptr10, "OK", "NULL", 10);
			flag = 20;
			onlie_flag = 0 ;
		}

		if (onlie_flag == 15)
		{
			onlie_flag = 0;
			sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":1}}\",0,0");
			ESP8266_Cmd(ptr10, "OK", "NULL", 10);
		}

		/*MAX30100*/
		Gain_Hr_Spo();

		/*ESP8266*/
		ESP8266_Task();
	}
}

void MPU6050_Task()
{
	onlie_flag++;
	MPU6050ReadAcc(ACC);
	SVM = sqrt((ACC[0] * ACC[0]) + (ACC[1] * ACC[1]) + (ACC[2] * ACC[2]));
	printf("SVM : %d\r\n", SVM);
	if (SVM<17000 & SVM> 14000)
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
		sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":0}}\",0,0");
		ESP8266_Cmd(ptr10, "OK", "NULL", 10);
		flag = 20;
	}

	if (onlie_flag == 15)
	{
		onlie_flag = 0;
		sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":1}}\",0,0");
		ESP8266_Cmd(ptr10, "OK", "NULL", 10);
	}
	/*使用MPU6050 DMP*/
	/*
		if (roll > 50 | roll < -60)
		{
			ILI9341_Clear(0, 40, 240, 40);
			ILI9341_DispString_EN(20, 40, "fall down(Euler Angle) : abnormal");
			flag1 = 250;

			sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":0}}\",0,0");
			ESP8266_Cmd(ptr10, "OK", "NULL", 10);
		}
		else
		{
			flag1--;
			if (flag1 == 0)
			{
				ILI9341_Clear(0, 40, 240, 40);
				ILI9341_DispString_EN(20, 40, "fall down : normal");
			}
			sprintf(ptr10, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"FallDown\\\":1}}\",0,0");
			ESP8266_Cmd(ptr10, "OK", "NULL", 10);
		}
		printf("pitch=%.2f   roll=%.2f   yaw=%.2f  \r\n", pitch, roll, yaw);
		*/
}

void ESP8266_Task(void)
{

	if (strUSART_Fram_Record.InfBit.FramFinishFlag == 1) // 如果接收到了串口调试助手的数据
	{
		strUSART_Fram_Record.Data_RX_BUF[strUSART_Fram_Record.InfBit.FramLength] = '\0';
		Usart_SendString(macESP8266_USARTx, strUSART_Fram_Record.Data_RX_BUF); // 数据从串口调试助手转发到ESP8266
		strUSART_Fram_Record.InfBit.FramLength = 0;							   // 接收数据长度置零
		strUSART_Fram_Record.InfBit.FramFinishFlag = 0;						   // 接收标志置零
	}
	if (strEsp8266_Fram_Record.InfBit.FramFinishFlag) // 如果接收到了ESP8266的数据
	{
		strEsp8266_Fram_Record.Data_RX_BUF[strEsp8266_Fram_Record.InfBit.FramLength] = '\0';
		Usart_SendString(DEBUG_USARTx, strEsp8266_Fram_Record.Data_RX_BUF); // 数据从ESP8266转发到串口调试助手
		strEsp8266_Fram_Record.InfBit.FramLength = 0;						// 接收数据长度置零
		strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;					// 接收标志置零
	}
}
