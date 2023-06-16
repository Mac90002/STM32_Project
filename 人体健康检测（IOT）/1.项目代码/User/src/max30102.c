#include "max30102.h"
#include "bsp_usart.h"
#include "stdbool.h"
#include "max30102_math.h"
#include "iicStart.h"

#define max30102_WR_address 0xAE
#define MAX_BRIGHTNESS 255

char temp2[200];
char temp1[200];
int32_t n_ir_buffer_length = 150; // data length
int32_t i, j, k, a;
uint32_t aun_ir_buffer[150];  // infrared LED sensor data
uint32_t aun_red_buffer[150]; // red LED sensor data
uint32_t un_min = 0x3fff, un_max = 0, un_prev_data, un_brightness;

int32_t n_spo2;		  // SPO2 value
int8_t ch_spo2_valid; // indicator to show if the SPO2 calculation is valid
int32_t n_heart_rate; // heart rate value
int8_t ch_hr_valid;	  // indicator to show if the heart rate calculation is valid
unsigned char manle;
float f_temp;

int32_t hrTimeout = 0;
int32_t hrValidCnt = 0;
int32_t hrThrowOutSamp = 0;
int32_t hr_buf[16];

int32_t hrBuffFilled;
int32_t hrSum;
int32_t hrAvg;

int32_t spo2Timeout = 0;
int32_t spo2ValidCnt = 0;
int32_t spo2ThrowOutSamp = 0;
int32_t spo2_buf[16];
int32_t spo2Sum;
int32_t spo2Avg;
int32_t spo2BuffFilled;
uint8_t uch_dummy;

bool maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)

{
	/* 第1步：发起I2C总线启动信号 */
	i2c_Start();

	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

	/* 第3步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("1失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第4步：发送字节地址 */
	i2c_SendByte(uch_addr);
	if (i2c_WaitAck() != 0)
	{
		printf("2失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第5步：开始写入数据 */
	i2c_SendByte(uch_data);

	/* 第6步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("3失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return true; /* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return false;
}

bool maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)

{
	/* 第1步：发起I2C总线启动信号 */
	i2c_Start();

	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

	/* 第3步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("4\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第4步：发送字节地址， */
	i2c_SendByte((uint8_t)uch_addr);
	if (i2c_WaitAck() != 0)
	{
		printf("5\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第6步：重新启动I2C总线。下面开始读取数据 */
	i2c_Start();

	/* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(max30102_WR_address | I2C_RD); /* 此处是读指令 */

	/* 第8步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("6\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第9步：读取数据 */
	{
		*puch_data = i2c_ReadByte(); /* 读1个字节 */

		i2c_NAck(); /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
	}
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return true; /* 执行成功 返回data值 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return false;
}

bool maxim_max30102_init(void)

{
	if (!maxim_max30102_write_reg(REG_INTR_ENABLE_1, 0xc0)) // INTR setting
	{
		printf("INTR1 setting false\r\n");
		return false;
	}

	if (!maxim_max30102_write_reg(REG_INTR_ENABLE_2, 0x00))
	{
		printf("INTR2 setting false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_FIFO_WR_PTR, 0x00)) // FIFO_WR_PTR[4:0]
	{
		printf("FIFO_WR_PTR[4:0] false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_OVF_COUNTER, 0x00)) // OVF_COUNTER[4:0]
	{
		printf("OVF_COUNTER[4:0] false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_FIFO_RD_PTR, 0x00)) // FIFO_RD_PTR[4:0]
	{
		printf("FIFO_RD_PTR[4:0] false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x6f)) // sample avg = 8, fifo rollover=false, fifo almost full = 17
	{
		printf("sample avg = 8, fifo rollover=false, fifo almost full = 17 false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x03)) // 0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
	{
		printf("0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x2F)) // SPO2_ADC range = 4096nA, SPO2 sample rate (400 Hz), LED pulseWidth (411uS)
	{
		printf("SPO2_ADC range = 4096nA, SPO2 sample rate (400 Hz), LED pulseWidth (411uS) false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_LED1_PA, 0x17)) // Choose value for ~ 4.5mA for LED1
	{
		printf("Choose value for ~ 4.5mA for LED1  false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_LED2_PA, 0x17)) // Choose value for ~ 4.5mA for LED2
	{
		printf("Choose value for ~ 4.5mA for LED2 false\r\n");
		return false;
	}
	if (!maxim_max30102_write_reg(REG_PILOT_PA, 0x7f)) // Choose value for ~ 25mA for Pilot LED
	{
		printf("Choose value for ~ 25mA for Pilot LED false\r\n");
		return false;
	}
	printf("MAX30102 Init true\r\n");
	return true;
}

bool maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)

{
	uint32_t un_temp;
	uint8_t uch_temp;
	*pun_ir_led = 0;
	*pun_red_led = 0;
	// printf("1_1\r\n");
	maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
	// printf("1_2\r\n");
	maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);
	// printf("1_3\r\n");

	/* 第1步：发起I2C总线启动信号 */
	i2c_Start();

	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

	/* 第3步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("7失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第4步：发送字节地址， */
	i2c_SendByte((uint8_t)REG_FIFO_DATA);
	if (i2c_WaitAck() != 0)
	{
		printf("8失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	/* 第6步：重新启动I2C总线。下面开始读取数据 */
	i2c_Start();

	/* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(max30102_WR_address | I2C_RD); /* 此处是读指令 */

	/* 第8步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		printf("9失败\r\n");
		goto cmd_fail; /* EEPROM器件无应答 */
	}

	un_temp = i2c_ReadByte();
	i2c_Ack();
	un_temp <<= 16;
	*pun_red_led += un_temp;
	un_temp = i2c_ReadByte();
	i2c_Ack();
	un_temp <<= 8;
	*pun_red_led += un_temp;
	un_temp = i2c_ReadByte();
	i2c_Ack();
	*pun_red_led += un_temp;

	un_temp = i2c_ReadByte();
	i2c_Ack();
	un_temp <<= 16;
	*pun_ir_led += un_temp;
	un_temp = i2c_ReadByte();
	i2c_Ack();
	un_temp <<= 8;
	*pun_ir_led += un_temp;
	un_temp = i2c_ReadByte();
	i2c_Ack();
	*pun_ir_led += un_temp;
	*pun_red_led &= 0x03FFFF; // Mask MSB [23:18]
	*pun_ir_led &= 0x03FFFF;  // Mask MSB [23:18]

	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return true;
cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return false;
}

bool maxim_max30102_reset()

{
	if (!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x40))
	{
		printf("10失败\r\n");
		return false;
	}
	else
		return true;
}

void Gain_Hr_Spo(void)
{
	i = 0;
	un_min = 0x3FFFF;
	un_max = 0;
	a++;
	// dumping the first 50 sets of samples in the memory and shift the last 100 sets of samples to the top
	for (i = 50; i < 150; i++)
	{
		aun_red_buffer[i - 50] = aun_red_buffer[i];
		aun_ir_buffer[i - 50] = aun_ir_buffer[i];

		// update the signal min and max
		if (un_min > aun_red_buffer[i])
			un_min = aun_red_buffer[i];
		if (un_max < aun_red_buffer[i])
			un_max = aun_red_buffer[i];
	}

	// take 50 sets of samples before calculating the heart rate.
	for (i = 100; i < 150; i++)
	{
		un_prev_data = aun_red_buffer[i - 1];
		maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i));

		// calculate the brightness of the LED
		if (aun_red_buffer[i] > un_prev_data)
		{
			f_temp = aun_red_buffer[i] - un_prev_data;
			f_temp /= (un_max - un_min);
			f_temp *= MAX_BRIGHTNESS;
			f_temp = un_brightness - f_temp;
			if (f_temp < 0)
				un_brightness = 0;
			else
				un_brightness = (int)f_temp;
		}
		else
		{
			f_temp = un_prev_data - aun_red_buffer[i];
			f_temp /= (un_max - un_min);
			f_temp *= MAX_BRIGHTNESS;
			un_brightness += (int)f_temp;
			if (un_brightness > MAX_BRIGHTNESS)
				un_brightness = MAX_BRIGHTNESS;
		}
	}
	maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

	if ((ch_hr_valid == 1) && (n_heart_rate < 190) && (n_heart_rate > 40))
	{
		hrTimeout = 0;

		// Throw out up to 1 out of every 5 valid samples if wacky
		if (hrValidCnt == 4)
		{
			hrThrowOutSamp = 1;
			hrValidCnt = 0;
			for (i = 12; i < 16; i++)
			{
				if (n_heart_rate < hr_buf[i] + 10)
				{
					hrThrowOutSamp = 0;
					hrValidCnt = 4;
				}
			}
		}
		else
		{
			hrValidCnt = hrValidCnt + 1;
		}

		if (hrThrowOutSamp == 0)
		{

			// Shift New Sample into buffer
			for (i = 0; i < 15; i++)
			{
				hr_buf[i] = hr_buf[i + 1];
			}
			hr_buf[15] = n_heart_rate;

			// Update buffer fill value
			if (hrBuffFilled < 16)
			{
				hrBuffFilled = hrBuffFilled + 1;
			}

			// Take moving average
			hrSum = 0;
			if (hrBuffFilled < 2)
			{
				hrAvg = 0;
			}
			else if (hrBuffFilled < 4)
			{
				for (i = 14; i < 16; i++)
				{
					hrSum = hrSum + hr_buf[i];
				}
				hrAvg = hrSum >> 1;
			}
			else if (hrBuffFilled < 8)
			{
				for (i = 12; i < 16; i++)
				{
					hrSum = hrSum + hr_buf[i];
				}
				hrAvg = hrSum >> 2;
			}
			else if (hrBuffFilled < 16)
			{
				for (i = 8; i < 16; i++)
				{
					hrSum = hrSum + hr_buf[i];
				}
				hrAvg = hrSum >> 3;
			}
			else
			{
				for (i = 0; i < 16; i++)
				{
					hrSum = hrSum + hr_buf[i];
				}
				hrAvg = hrSum >> 4;
			}
		}
		hrThrowOutSamp = 0;
	}
	else
	{
		hrValidCnt = 0;
		if (hrTimeout == 4)
		{
			hrAvg = 0;
			hrBuffFilled = 0;
		}
		else
		{
			hrTimeout++;
		}
	}

	if ((ch_spo2_valid == 1) && (n_spo2 > 59))
	{
		spo2Timeout = 0;

		// Throw out up to 1 out of every 5 valid samples if wacky
		if (spo2ValidCnt == 4)
		{
			spo2ThrowOutSamp = 1;
			spo2ValidCnt = 0;
			for (i = 12; i < 16; i++)
			{
				if (n_spo2 > spo2_buf[i] - 10)
				{
					spo2ThrowOutSamp = 0;
					spo2ValidCnt = 4;
				}
			}
		}
		else
		{
			spo2ValidCnt = spo2ValidCnt + 1;
		}

		if (spo2ThrowOutSamp == 0)
		{

			// Shift New Sample into buffer
			for (i = 0; i < 15; i++)
			{
				spo2_buf[i] = spo2_buf[i + 1];
			}
			spo2_buf[15] = n_spo2;

			// Update buffer fill value
			if (spo2BuffFilled < 16)
			{
				spo2BuffFilled = spo2BuffFilled + 1;
			}

			// Take moving average
			spo2Sum = 0;
			if (spo2BuffFilled < 2)
			{
				spo2Avg = 0;
			}
			else if (spo2BuffFilled < 4)
			{
				for (i = 14; i < 16; i++)
				{
					spo2Sum = spo2Sum + spo2_buf[i];
				}
				spo2Avg = spo2Sum >> 1;
			}
			else if (spo2BuffFilled < 8)
			{
				for (i = 12; i < 16; i++)
				{
					spo2Sum = spo2Sum + spo2_buf[i];
				}
				spo2Avg = spo2Sum >> 2;
			}
			else if (spo2BuffFilled < 16)
			{
				for (i = 8; i < 16; i++)
				{
					spo2Sum = spo2Sum + spo2_buf[i];
				}
				spo2Avg = spo2Sum >> 3;
			}
			else
			{
				for (i = 0; i < 16; i++)
				{
					spo2Sum = spo2Sum + spo2_buf[i];
				}
				spo2Avg = spo2Sum >> 4;
			}
		}
		spo2ThrowOutSamp = 0;
	}
	else
	{
		spo2ValidCnt = 0;
		if (spo2Timeout == 4)
		{
			spo2Avg = 0;
			spo2BuffFilled = 0;
		}
		else
		{
			spo2Timeout++;
		}
	}

	// printf("%d %d\r\n",hrAvg,spo2Avg);
	if (a == 20)
	{
		a = 0;
		sprintf(temp2, "AT+MQTTPUB=0,\"/sys/iv8n2SoV9vD/D001/thing/event/property/post\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\,\\\"id\\\":\\\"00001\\\"\\,\\\"params\\\":{\\\"heart\\\":%d}}\",0,0", hrAvg);
		ESP8266_Cmd(temp2, "OK", "NULL", 2000);
	}
	sprintf(temp1, "heart rate :%d", hrAvg);
	ILI9341_Clear(0, 60, 240, 20);
	SysTick_Delay_Ms(10);
	ILI9341_DispString_EN(20, 60, temp1);
}

void max3012_init()
{
	bsp_InitI2C(); // 初始化IIC接口

	maxim_max30102_reset(); // resets the MAX30102

	maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_dummy);
	// Reads/clears the interrupt status register
	maxim_max30102_init(); // initialize the MAX30102

	// read the first 150 samples, and determine the signal range
	for (i = 0; i < n_ir_buffer_length; i++) // n_ir_buffer_length
	{
		maxim_max30102_read_fifo(&aun_red_buffer[i], &aun_ir_buffer[i]); // read from MAX30102 FIFO
		if (un_min > aun_red_buffer[i])
			un_min = aun_red_buffer[i]; // update signal min
		if (un_max < aun_red_buffer[i])
			un_max = aun_red_buffer[i]; // update signal max
	}
	un_prev_data = aun_red_buffer[i];
	// calculate heart rate and SpO2 after first 150 samples (first 3 seconds of samples)
	maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
	printf("ok");
}
