#include "bsp_key.h"

/**
 * @brief  配置按键用到的I/O口
 * @param  无
 * @retval 无
 */
void Key_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/*开启按键端口的时钟*/
	RCC_APB2PeriphClockCmd(KEY1_GPIO_CLK | KEY2_GPIO_CLK, ENABLE);

	// 选择按键的引脚
	GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_PIN;
	// 设置按键的引脚为浮空输入
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// 使用结构体初始化按键
	GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);

	// 选择按键的引脚
	GPIO_InitStructure.GPIO_Pin = KEY2_GPIO_PIN;
	// 设置按键的引脚为浮空输入
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// 使用结构体初始化按键
	GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * 函数名：Key_Scan
 * 描述  ：检测是否有按键按下
 * 输入  ：GPIOx：x 可以是 A，B，C，D或者 E
 *		     GPIO_Pin：待读取的端口位
 * 输出  ：KEY_OFF(没按下按键)、KEY_ON（按下按键）
 */
uint8_t Key_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	/*检测是否有按键按下 */
	if (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == KEY_ON)
	{
		/*等待按键释放 */
		while (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == KEY_ON)
			;
		return KEY_ON;
	}
	else
		return KEY_OFF;
}
/*********************************************END OF FILE**********************/
static void NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_Initstruct;

	// 配置 NVIC KEY1
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_Initstruct.NVIC_IRQChannel = KEY1_NVIC_IRQN;
	NVIC_Initstruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Initstruct.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_Initstruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_Initstruct);

	NVIC_Initstruct.NVIC_IRQChannel = KEY2_NVIC_IRQN;
	NVIC_Initstruct.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_Initstruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Initstruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_Initstruct);
}

void BSP_KEY_EXTI_Config(void)
{
	GPIO_InitTypeDef GPIO_Initstruct;
	EXTI_InitTypeDef EXIT_Initstruct;
	NVIC_Config();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// 初始化GPIOA KEY1
	GPIO_Initstruct.GPIO_Pin = KEY1_GPIO_PIN;
	GPIO_Initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(KEY1_GPIO_PORT, &GPIO_Initstruct);
	// GPIO_ResetBits(KEY1_GPIO_PORT,KEY1_GPIO_PIN);

	// 初始化 EXTI	KEY1 选择 EXTI的信号源
	GPIO_EXTILineConfig(KEY1_EXTI_PortSource, KEY1_EXTI_Pin);
	EXIT_Initstruct.EXTI_Line = KEY1_EXTI_LINE;
	EXIT_Initstruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXIT_Initstruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXIT_Initstruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXIT_Initstruct);

	// 初始化GPIOC  KEY2
	GPIO_Initstruct.GPIO_Pin = KEY2_GPIO_PIN;
	GPIO_Initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(KEY2_GPIO_PORT, &GPIO_Initstruct);
	// GPIO_ResetBits(KEY2_GPIO_PORT,KEY2_GPIO_PIN);

	// 初始化 EXTI KEY2
	GPIO_EXTILineConfig(KEY2_EXTI_PortSource, KEY2_EXTI_Pin);
	EXIT_Initstruct.EXTI_Line = KEY2_EXTI_LINE;
	EXIT_Initstruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXIT_Initstruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXIT_Initstruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXIT_Initstruct);
}
