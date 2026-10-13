/********************************************************************************
* @file		:my_timer.h
* @brief	:my_timer.c 头文件
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_TIMER_H__
#define __MY_TIMER_H__

/** 
  * @brief  TIMER 定时器引脚重映射配置(默认)
  */
#define __TIM1_AFIO     __HAL_AFIO_REMAP_TIM1_DISABLE
#define __TIM2_AFIO     __HAL_AFIO_REMAP_TIM2_DISABLE
#define __TIM3_AFIO     __HAL_AFIO_REMAP_TIM3_DISABLE
#define __TIM4_AFIO     __HAL_AFIO_REMAP_TIM4_DISABLE


/** 
  * @brief  TIMER 定时器通道定义
  */
typedef enum
{
	TIM_CHN1 = 0x01,
	TIM_CHN2 = 0x02,
	TIM_CHN3 = 0x04,
	TIM_CHN4 = 0x08,
}my_timer_chn_t;

/** 
  * @brief  TIMER 定时器工作模式定义
  */
typedef enum
{
	TIM_MODE_BASE = 0,
	TIM_MODE_PWM,
	TIM_MODE_ENCODER,
	TIM_MODE_IC,
	TIM_MODE_ONEPULSE,
	TIM_MODE_ETR,
}my_timer_mode_t;

/** 
* @brief STM32片上外设 -- TIM定时器对象，单例模型结构体定义
  */
typedef struct _MY_TIMER
{
    /************************************************* 
    * Description: 定时器基础定时模式初始化
    * Input : <xId> 定时器ID @ my_timer_id
			  <psc> 预分频系数
			  <arr> 自动重装载值
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		`ret = my_timer.base_init(TIM_ID1);
    *************************************************/  
	int(*base_init)(my_timer_id xId, uint16_t psc, uint16_t arr);

	/************************************************* 
    * Description: 定时器PWM模式初始化
    * Input : <xId> 定时器ID @ my_timer_id; 
			  <psc> 预分频系数
			  <arr> 自动重装载值
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = my_timer.pwm_init(TIM_ID1, 71, 1000, TIM_CHN1|TIM_CHN2|TIM_CHN3|TIM_CHN4);
    *************************************************/ 
	int(*pwm_init)(my_timer_id xId, uint16_t psc, uint16_t arr, uint8_t Channels);
	
	/************************************************* 
    * Description: 定时器PWM输出配置
    * Input : <xId> 定时器ID @ my_timer_id; 
			  <Channel> 定时器通道，每次只能设置一个通道;
			  <DutyCycle> 占空比;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = my_timer.pwm_set(TIM_ID1, TIM_CHN1|TIM_CHN2|TIM_CHN3|TIM_CHN4, 50);
    *************************************************/ 
	int(*pwm_set)(my_timer_id xId, uint8_t Channels, uint32_t DutyCycle);
}c_my_timer;


/** 
  * @brief  片上ADC外设, 单例模型，扩展到外部使用
  */
extern const c_my_timer my_timer;


#endif 

