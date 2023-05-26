#ifndef __BSP_HC05_H
#define __BSP_HC05_H

#include "stm32f10x.h"

#define HC05_USART                  USART3

#define BLT_INT_GPIO_PORT    	    GPIOC			              /* GPIO端口 */
#define BLT_INT_GPIO_CLK 	        RCC_APB2Periph_GPIOC		/* GPIO端口时钟 */
#define BLT_INT_GPIO_PIN		  		GPIO_Pin_4		          /* 连接到HC05 INT引脚的GPIO */

#define BLT_KEY_GPIO_PORT    	    GPIOG			              /* GPIO端口 */
#define BLT_KEY_GPIO_CLK 	        RCC_APB2Periph_GPIOG		/* GPIO端口时钟 */
#define BLT_KEY_GPIO_PIN		    	GPIO_Pin_8		          /* 连接到HC05 KEY引脚的GPIO */

#define BLT_KEY_HIGHT  		        GPIO_SetBits(BLT_KEY_GPIO_PORT, BLT_KEY_GPIO_PIN);
#define BLT_KEY_LOW  							GPIO_ResetBits(BLT_KEY_GPIO_PORT, BLT_KEY_GPIO_PIN);
#define IS_HC05_CONNECTED() 	    GPIO_ReadInputDataBit(BLT_INT_GPIO_PORT,BLT_INT_GPIO_PIN)

//最大蓝牙设备数量
#define BLTDEV_MAX_NUM 10

                                       
                                       
/*蓝牙地址，数字形式，分NAP，UAP，LAP段*/																			 

typedef  struct 
{
	uint8_t num;		//扫描到的蓝牙设备数量
		
	char unpraseAddr[BLTDEV_MAX_NUM][50];	//蓝牙设备地址，字符串形式，方便扫描时和连接时使用
	
	char name[BLTDEV_MAX_NUM][50];	//蓝牙设备的名字
	
}BLTDev;

//蓝牙设备列表，在 bsp_hc05.c 文件中定义
extern  BLTDev bltDevList;


enum
{
  HC05_DEFAULT_TIMEOUT = 200,
  HC05_INQUIRY_DEFAULT_TIMEOUT = 10000,
  HC05_PAIRING_DEFAULT_TIMEOUT = 10000,
  HC05_PASSWORD_MAXLEN = 16,
  HC05_PASSWORD_BUFSIZE = HC05_PASSWORD_MAXLEN + 1,
  HC05_NAME_MAXLEN = 32,
  HC05_NAME_BUFSIZE = HC05_NAME_MAXLEN + 1,
  HC05_ADDRESS_MAXLEN = 14,
  HC05_ADDRESS_BUFSIZE = HC05_ADDRESS_MAXLEN + 1,
};


uint8_t HC05_Init(void);
uint8_t HC05_Send_CMD(char* cmd , uint8_t clean);
void TransData_CtrlLED_Test(void);
#endif /* __BSP_HC05_H */

