#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "bsp_usart.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_SysTick.h"
#include "bsp_task.h"
#include "bsp_hc05.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_as608.h"
#include "rx_data_queue.h"

char hc05_nameCMD[40];
char disp_buff[200];
char hc05_name[30]="HC05_Module";
int main()
{

    SysTick_Init();
		rx_queue_init();
    LED_GPIO_Config();
    Key_GPIO_Config();
    USART_Config();
		ILI9341_Config();
    AS608_Config();

    ILI9341_Clear(0,0,240,320);
	
	if(HC05_Init() == 0){
      printf("HC05 Succeed Connect\r\n");
      ILI9341_DispString_EN(20,40,"HC05 Succeed Connect"); 
    }
    else{
      printf("HC05 Failed Connect\r\n");
      ILI9341_DispString_EN(20,40,"HC05 Failed Connect");
      ILI9341_DispString_EN(20,80,"Please Cheak Connect");
    }
		
		
		HC05_Send_CMD("AT+INIT\r\n",1); // 初始化SPP库
		HC05_Send_CMD("AT+CLASS=0\r\n",1);// 查询各种蓝牙设备
		HC05_Send_CMD("AT+INQM=1,9,48\r\n",1);// 超过9个蓝牙设备响应则中止查询
		
		sprintf(hc05_nameCMD,"AT+NAME=%s\r\n",hc05_name);
		HC05_Send_CMD(hc05_nameCMD,1);
		
		sprintf(disp_buff,"Device name:%s",hc05_name);
		ILI9341_DispString_EN(20,60,disp_buff);
	
    OS_Task();
}

