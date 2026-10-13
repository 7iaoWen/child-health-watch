/********************************************************************************
* @file		:mpu6050.h
* @brief	:mpu6050.c头文件
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __MPU6050_H__
#define __MPU6050_H__

/* Public Macros  -------------------------------------------------------------------*/
typedef enum{
	_2G = 0,
	_4G,
	_8G,
	_16G,
}accel_range_t;

typedef enum{
	_250_DPS = 0,
	_500_DPS,
	_1000_DPS,
	_2000_DPS,
}gyro_range_t;

#define MPU6050_IIC_ADDR (0x68)      //MPU6050器件地址 (读地址： 0x68<<1|0； 写地址： 0x68<<1|1)  

#define GYRO_RANGE	_2000_DPS
#define ACC_RANGE	_2G
/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief MPU6050类，结构体定义 
  */
typedef struct _MPU6050 c_mpu6050;
typedef struct _MPU6050
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;

	/************************************************* 
    * Description: MPU6050 获取加速度值
    * Input : <p_obj> MPU6050 对象;
	 		  <facc> 用于保存获取到的加速度值;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
	 		float facc[3];
			ret = mpu6050.read_acc(&mpu6050, facc);
    *************************************************/ 
	int(*read_acc)(const c_mpu6050 *p_obj, float *facc);

	/************************************************* 
    * Description: MPU6050 获取角加速度值
    * Input : <p_obj> MPU6050 对象;
			  <fgyro> 用于保存获取到的角加速度值;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
	 		float fgyro[3];
			ret = mpu6050.read_gyro(&mpu6050, fgyro);
    *************************************************/ 
	int(*read_gyro)(const c_mpu6050 *p_obj, float *fgyro);
	
	/************************************************* 
    * Description: MPU6050 获取角度值
    * Input : <p_obj> MPU6050 对象;
			  <angle> 用于保存获取到的角加速度值;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			float angle[3];
			ret = mpu6050.read_angle(&mpu6050, angle);
    *************************************************/ 
	int(*read_angle)(const c_mpu6050 *p_obj, float *angle);
	
}c_mpu6050;


/************************************************* 
* Description: 创建一个 MPU6050 对象
* Input : <iic> 对象;
* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
* Demo  : 
	c_my_iic iic = my_iic_create(GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1);
	c_mpu6050 mpu6050 = mpu6050_create(&iic);
*************************************************/ 
c_mpu6050 mpu6050_create(c_my_iic *iic);


#endif 

