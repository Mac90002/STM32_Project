#include "bsp_hc05.h"
#include "bsp_hc05_usart.h"
#include "bsp_SysTick.h"
#include "bsp_usart.h"
#include "bsp_led.h"
#include "bsp_ili9341_lcd.h"
#include <string.h>

BLTDev bltDevList;
extern ReceiveData DEBUG_USART_ReceiveData;
extern ReceiveData BLT_USART_ReceiveData;

extern uint8_t Clear_Message ;

static void HC05_GPIO_Config(void)
{		
		
    GPIO_InitTypeDef GPIO_InitStructure;

    /*开启GPIO时钟*/
    RCC_APB2PeriphClockCmd( BLT_INT_GPIO_CLK|BLT_KEY_GPIO_CLK, ENABLE); 

    GPIO_InitStructure.GPIO_Pin = BLT_INT_GPIO_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(BLT_INT_GPIO_PORT, &GPIO_InitStructure);	

    
    GPIO_InitStructure.GPIO_Pin = BLT_KEY_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(BLT_KEY_GPIO_PORT, &GPIO_InitStructure);	
}

/// @brief 发送AT命令 只适用于具有OK应答的命令
/// @param cmd 命令 需要后缀\r\n
/// @param clean 是否清除接收缓冲区 1 清除 0 不清除
/// @return 0 AT发送成功 1 失败
uint8_t HC05_Send_CMD(char* cmd , uint8_t clean){
    uint8_t     retry=3; // 尝试3次
    uint16_t    i = 0 ;
    uint16_t    len;
    char *      redata;

    while(retry --){
        BLT_KEY_HIGHT; // 拉高KEY引脚 
        Usart_SendString(HC05_USART,(uint8_t *)cmd);
        Delay_ms(10);
        i = 500 ;
        do{
            redata = get_rebuff(&len);
            if(len>0)
                {
                    if(strstr(redata,"OK"))				
                    {
                        printf("send CMD: %s",cmd); //打印发送的蓝牙指令和返回信息

                        printf("recv back: %s",redata);
                        
                        if(clean==1)
                            clean_rebuff();
                        return 0; //AT指令成功
                    }
                }

               Delay_ms(10); 
                
          }while(i--);
          printf("send CMD: %s",cmd); //打印发送的蓝牙指令和返回信息
          printf("recv back: %s",redata);
          printf("HC05 send CMD fail %d times", retry); //提示失败重试
    }

    printf("HC05 send CMD fail ");
		
	if(clean==1)
		clean_rebuff();

	return 1; //AT指令失败 
}


uint8_t HC05_Init(){
    
    HC05_USART_Init();
    HC05_GPIO_Config();
    return HC05_Send_CMD("AT\r\n",1);

}

void TransData_CtrlLED_Test(void)
{
  /* 处理蓝牙串口接收到的蓝牙数据 */
  if(BLT_USART_ReceiveData.receive_data_flag == 1)
  {
    BLT_USART_ReceiveData.uart_buff[BLT_USART_ReceiveData.datanum] = 0;
    //在这里可以自己定义想要接收的字符串然后处理
    //这里接收到手机蓝牙发来的 “1234”就会把板子上面的红灯取反一次
    if(strstr((char *)BLT_USART_ReceiveData.uart_buff,"1234"))
    {
            
			LED2_TOGGLE;
			Clear_Message = 0 ;
			ILI9341_Clear(0,0,240,320);
			ILI9341_DispString_EN(20,40,"your password is right !!!");
      ILI9341_DispString_EN(20,60,"The door is already open !!!");
      SysTick_Delay_Ms(4000);
    }
		
		if(!strstr((char *)BLT_USART_ReceiveData.uart_buff,"1234"))
    {
			LED2_OFF;
			ILI9341_Clear(0,0,240,320);
			ILI9341_DispString_EN(20,40,"password is failed !!!");
			SysTick_Delay_Ms(4000);
    }
    
    //串口助手显示接收到的数据
    Usart_SendString( DEBUG_USARTx, "\r\nrecv HC-05 data:\r\n" );
    Usart_SendString( DEBUG_USARTx, BLT_USART_ReceiveData.uart_buff );
    Usart_SendString( DEBUG_USARTx, "\r\n" );
    
    //清零蓝牙串口数据缓存
    BLT_USART_ReceiveData.receive_data_flag = 0;		//接收数据标志清零
    BLT_USART_ReceiveData.datanum = 0;  
  }
}



