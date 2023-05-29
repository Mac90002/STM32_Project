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

int main()
{

  SysTick_Init();
  rx_queue_init();
  LED_GPIO_Config();
  Key_GPIO_Config();
  USART_Config();
  ILI9341_Config();
  AS608_Config();

  OS_Task();
}
