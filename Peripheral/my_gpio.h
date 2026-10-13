/********************************************************************************
* @file		:my_gpio.h
* @brief	:my_gpio.c头文件
* @Author	:Gwen9
* @Date		:2024/08/01
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_GPIO_H__
#define __MY_GPIO_H__

/** 
  * @brief  GPIO 初始化类型，输入/输出 
  */
typedef enum
{ 
  GPIO_OUT = 0,
  GPIO_IN
}GPIO_INIT_TYPE;

typedef struct _MY_GPIO c_my_gpio;
typedef struct _MY_GPIO
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
	void* this;  
	    
    /************************************************* 
    * Description: 设置IO输出的电平状态
    * Input : <p_obj> IO对象; <state> IO口状态 
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		`ret = gpio.set(&gpio, GPIO_PIN_RESET);
    *************************************************/  
	int(*set)(const c_my_gpio* p_obj, GPIO_PinState state);

	/************************************************* 
    * Description: 翻转IO输出的电平状态
    * Input : <p_obj> IO对象;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = gpio.toggle(&gpio);
    *************************************************/  
	int(*toggle)(const c_my_gpio* p_obj);
	
	/************************************************* 
    * Description: 获取IO口输入的电平状态
    * Input : <p_obj> IO对象; 
    * Output: <*state> IO口状态返回的地址;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		GPIO_PinState state;
		ret = gpio.get(&gpio, &state);
    *************************************************/  
	int(*get)(const c_my_gpio* p_obj, GPIO_PinState *state);
}c_my_gpio;
	
/** 
* @brief STM32片上外设 -- GPIO单例模型结构体定义
  */
typedef struct _MY_GPIO_T
{
    /************************************************* 
    * Description: 片上ADC外设初始化程序
    * Input : <xChn> 需要初始化的ADC通道，@defgroup ADC_channels;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		my_gpio.init(GPIOA, GPIO_PIN_0, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);	
    *************************************************/  
	int(*init)(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t Mode, uint32_t Pull, uint32_t Speed);
}c_my_gpio_t;



/************************************************* 
* Description: 新建一个GPIO对象
* Input : <GPIOx> GPIO端口号; <GPIO_Pin> GPIO引脚序号; <type> 输入/输出类型;
* Output: 无
* Return: <E_OK> 操作成功; <E_ERROR> 操作失败
* Demo  : 
	c_my_gpio led = my_gpio_create(GPIOA, GPIO_PIN_0, GPIO_OUT);
	if(led == NULL)	error();
*************************************************/  
c_my_gpio my_gpio_create(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_INIT_TYPE type);

/** 
  * @brief  片上GPIO外设, 单例模型，扩展到外部使用
  */
extern const c_my_gpio_t my_gpio;

#endif 
