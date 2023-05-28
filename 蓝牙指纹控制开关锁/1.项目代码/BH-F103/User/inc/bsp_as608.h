#ifndef __BSP_AS608_H
#define __BSP_AS608_H

#include "stm32f10x.h"

#define AS608_DELAY_MS(x) SysTick_Delay_Ms(x)
#define CHAR_BUFFER1 0x01
#define CHAR_BUFFER2 0x02
#define PS_MAXNUM 240 /*指纹模块最大存储指纹模板数*/


/*串口2-USART2*/
#define  AS608_USART                    USART2
#define  AS608_USART_CLK                RCC_APB1Periph_USART2
#define  AS608_USART_APBxClkCmd         RCC_APB1PeriphClockCmd
#define  AS608_USART_BAUDRATE           57600

/*USART GPIO 引脚宏定义*/
#define  AS608_USART_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define  AS608_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
    
#define  AS608_USART_TX_GPIO_PORT       GPIOA   
#define  AS608_USART_TX_GPIO_PIN        GPIO_Pin_2
#define  AS608_USART_RX_GPIO_PORT       GPIOA
#define  AS608_USART_RX_GPIO_PIN        GPIO_Pin_3

#define  AS608_USART_IRQ                USART2_IRQn
#define  AS608_USART_IRQHandler         USART2_IRQHandler


/*TouchOut引脚定义*/
#define AS608_TouchOut_INT_GPIO_PORT         GPIOA
#define AS608_TouchOut_INT_GPIO_CLK          (RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO)
#define AS608_TouchOut_INT_GPIO_PIN          GPIO_Pin_8
#define AS608_TouchOut_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOA
#define AS608_TouchOut_INT_EXTI_PINSOURCE    GPIO_PinSource8
#define AS608_TouchOut_INT_EXTI_LINE         EXTI_Line8
#define AS608_TouchOut_INT_EXTI_IRQ          EXTI9_5_IRQn

#define AS608_TouchOut_IRQHandler            EXTI9_5_IRQHandler

void AS608_Config(void);
void AS608_Connect_Test(void);
uint16_t PS_GetImage(void);
uint16_t PS_GenChar(uint8_t BufferID);
uint16_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, uint16_t *p);
void AS608_TASK(void);



#endif /* __BSP_AS608_H */

