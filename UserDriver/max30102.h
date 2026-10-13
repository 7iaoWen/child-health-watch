/********************************************************************************
* @file		:max30102.h
* @brief	:max30102.c头文件
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __MAX30102_H__
#define __MAX30102_H__

/* Public Macros  -------------------------------------------------------------------*/
/*定义采样频率和采样时间以及缓存大小*/
#define FS 				50    //sampling frequency
#define BUFFER_SIZE  	(FS* 3) 

/*连续多少次数据无效数据，认为是测量结束*/
#define MAX30102_INVAILD_MAX	10

// #define MAX30102_IIC_ADDR (0xAE)      //MAX30102器件地址 (写地址： 0xAE|0； 读地址： 0xAE|1)  
//注意：如果使用 Peripheral 库中的模拟iic,传入的iic地址只能是 7bits 的， 所以这里需要转换一下
#define MAX30102_IIC_ADDR (0x57)      	//MAX30102器件地址

/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief MAX30102类，结构体定义 
  */
typedef struct _MAX30102 c_max30102;
typedef struct _MAX30102
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;

	/************************************************* 
    * Description: MAX30102 获取心率和血氧浓度值 (因为需要采集大量数据进行滤波处理所以此函数调用周期尽量在10ms以内)
    * Input : <p_obj> MAX30102 对象;
	 		  <facc> 用于保存获取到的加速度值;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			uint32_t heartrate, o2;
			ret = max30102.get(&max30102, &heartrate, &o2);
    *************************************************/ 
	 int(*get)(const c_max30102* p_obj, uint32_t* heartrate, uint32_t* o2);

}c_max30102;


/************************************************* 
* Description: 创建一个 MAX30102 对象
* Input : <gpio> MAX30102 对象;
		  <pin> 用于保存获取到的温度数据;
* Output: 无
* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
* Demo  : 
	uint32_t red_buf[BUFFER_SIZE];
	uint32_t ir_buf[BUFFER_SIZE];
	c_my_iic iic = my_iic_create(GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1);
	c_max30102 max30102 = max30102_create(&iic, red_buf, ir_buf);
*************************************************/ 
c_max30102 max30102_create(c_my_iic *iic, uint32_t *red_buf, uint32_t *ir_buf);


#endif 

