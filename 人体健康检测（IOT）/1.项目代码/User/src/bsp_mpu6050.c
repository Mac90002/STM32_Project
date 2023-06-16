#include "bsp_mpu6050.h"
#include "IIC.h"
#include "bsp_usart.h"
#include "inv_mpu.h"

void MPU6050_Init(void)
{
    iic_init();
    int i = 0, j = 0;
    // 在初始化之前要延时一段时间，若没有延时，则断电后再上电数据可能会出错
    for (i = 0; i < 1000; i++)
    {
        for (j = 0; j < 1000; j++)
        {
        }
    }
    MPU6050_WriteReg(MPU6050_RA_PWR_MGMT_1, 0x00);   // 解除休眠状态                                             reg 107
    MPU6050_WriteReg(MPU6050_RA_SMPLRT_DIV, 0x07);   // 陀螺仪采样率，1KHz                                       reg25
    MPU6050_WriteReg(MPU6050_RA_CONFIG, 0x06);       // 低通滤波器的设置，截止频率是1K，带宽是5K                  reg26
    MPU6050_WriteReg(MPU6050_RA_ACCEL_CONFIG, 0x00); // 配置加速度传感器工作在2G模式，不自检                      reg28
    MPU6050_WriteReg(MPU6050_RA_GYRO_CONFIG, 0x18);  // 陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)     reg27
    MPU6050_WriteReg(0x38, 0x01);
    MPU6050_WriteReg(0x37, 0x80);

    // mpu_dmp_init();
}

/// @brief 往寄存器里写数据
/// @param reg_add 寄存器地址
/// @param reg_dat 数据
void MPU6050_WriteReg(u8 reg_add, u8 reg_dat)
{
    iic_start();
    iic_send_byte(MPU6050_SLAVE_ADDRESS); // 地址+写
    iic_wait_ack();
    iic_send_byte(reg_add);
    iic_wait_ack();
    iic_send_byte(reg_dat);
    iic_wait_ack();
    iic_stop();
}

/// @brief 从寄存器中读数据
/// @param reg_add 寄存器地址
/// @param Read 读缓存
/// @param num 读几个数据
void MPU6050_ReadData(u8 reg_add, unsigned char *Read, u8 num)
{
    uint8_t i;
    iic_start();
    iic_send_byte(MPU6050_SLAVE_ADDRESS);
    iic_wait_ack();
    iic_send_byte(reg_add);
    iic_wait_ack();
    iic_start();
    iic_send_byte(MPU6050_SLAVE_ADDRESS + 1); // 地址+读
    iic_wait_ack();
    for (i = 0; i <= (num - 1); i++)
    {
        *Read = iic_read_byte(1);
        Read++;
    }
    iic_stop();
}

uint8_t MPU6050ReadID(void)
{
    unsigned char Re = 0;
    MPU6050_ReadData(MPU6050_RA_WHO_AM_I, &Re, 1); // 读器件地址
    if (Re != 0x68)
    {
        printf("MPU6050 dectected error!\r\n检测不到MPU6050模块,请检查模块与开发板的接线");
        return 0;
    }
    else
    {
        printf("MPU6050 ID = %#x\r\n", Re);
        return 1;
    }
}

/// @brief 温度测量值 reg65 66 type :read only
/// @param tempData
void MPU6050ReadTemp(short *tempData)
{
    uint8_t buf[2];
    MPU6050_ReadData(MPU6050_RA_TEMP_OUT_H, buf, 2); // 读取温度值
    *tempData = (buf[0] << 8) | buf[1];
}

/// @brief 生成摄氏度
/// @param Temperature
void MPU6050_ReturnTemp(float *Temperature)
{
    short temp3;
    u8 buf[2];

    MPU6050_ReadData(MPU6050_RA_TEMP_OUT_H, buf, 2); // 读取温度值
    temp3 = (buf[0] << 8) | buf[1];
    *Temperature = ((double)temp3 / 340.0) + 36.53;
}

/// @brief 读取测量值 角加速度
/// @param gyroData
void MPU6050ReadGyro(short *gyroData)
{
    u8 buf[6];
    MPU6050_ReadData(MPU6050_GYRO_OUT, buf, 6);
    gyroData[0] = (buf[0] << 8) | buf[1]; // X轴测量值
    gyroData[1] = (buf[2] << 8) | buf[3]; // Y轴测量值
    gyroData[2] = (buf[4] << 8) | buf[5]; // Z轴测量值
}

void MPU6050ReadAcc(short *accData)
{
    u8 buf[6];
    MPU6050_ReadData(MPU6050_ACC_OUT, buf, 6);
    accData[0] = (buf[0] << 8) | buf[1]; // X轴测量值
    accData[1] = (buf[2] << 8) | buf[3]; // Y轴测量值
    accData[2] = (buf[4] << 8) | buf[5]; // Z轴测量值
}
