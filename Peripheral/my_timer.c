/********************************************************************************
* @file		:my_timer.c
* @brief	:对STM32 HAL 库中 TIM 定时器的二次封装，提供 TIMER 的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_TIMER_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/
typedef struct __TIMER_CFG
{
    TIM_TypeDef*  	BASE;
	struct{
		GPIO_TypeDef* 	GPIOx;
		uint16_t 		GPIO_Pin;
	}Chn[4];
}timer_cfg_t;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_timer_base_init(my_timer_id xId, uint16_t psc, uint16_t arr);
static int my_timer_pwm_init(my_timer_id xId, uint16_t psc, uint16_t arr, uint8_t Channels);
static int my_timer_pwm_set(my_timer_id xId, uint8_t Channels, uint32_t DutyCycle);

/* Private Variables -------------------------------------------------------------------*/
//参数定义: 定时器单例模型，扩展到外部使用定义
const c_my_timer my_timer = {my_timer_base_init, my_timer_pwm_init, my_timer_pwm_set};
//参数定义: 定时器通道默认引脚
static const timer_cfg_t TIM_CFG[4] = {
	{TIM1,     GPIOA, GPIO_PIN_12,    GPIOA, GPIO_PIN_8,     GPIOA, GPIO_PIN_9,    GPIOA, GPIO_PIN_10},
	{TIM2,     GPIOA, GPIO_PIN_0, 	  GPIOA, GPIO_PIN_1,     GPIOA, GPIO_PIN_2,     GPIOA, GPIO_PIN_3},
	{TIM3,     GPIOA, GPIO_PIN_6, 	  GPIOA, GPIO_PIN_7,     GPIOB, GPIO_PIN_0,     GPIOB, GPIO_PIN_1},
	{TIM4,     GPIOB, GPIO_PIN_6, 	  GPIOB, GPIO_PIN_7,     GPIOB, GPIO_PIN_8,     GPIOB, GPIO_PIN_9},
};
//参数定义： 定时器初始化状态
static bool _timer_init_state[4] = {false};
//参数定义： 定时器句柄
static TIM_HandleTypeDef htim[MY_TIMER_MAX];
 
/* Private Functions -------------------------------------------------------------------*/
/** 
 *@brief 定时器基础定时模式初始化
 */
static int my_timer_base_init(my_timer_id xId, uint16_t psc, uint16_t arr)
{
	/*base init*/
	TIM_HandleTypeDef  *htim_temp     = &htim[xId];
    htim_temp->Instance               = TIM_CFG[xId].BASE;              /*设备及基地址*/
    htim_temp->Init.Prescaler         = psc;             				/*预分频系数*/
    htim_temp->Init.CounterMode       = TIM_COUNTERMODE_UP;            	/*计数模式:向上计数*/
    htim_temp->Init.Period            = arr;        					/*自动装载值*/
    htim_temp->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;       	/*定时器时钟分频*/
//    htim_temp->Init.RepetitionCounter =                               /*重复计数器*/
    htim_temp->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;	/*自动重装载*/

	HAL_TIM_Base_Init(htim_temp); 		/*初始化*/  
	HAL_TIM_Base_Start_IT(htim_temp); 	/*开启中断*/		
    return R_OK;
}

/** 
 *@brief 定时器PWM模式初始化
 */
static int my_timer_pwm_init(my_timer_id xId, uint16_t psc, uint16_t arr, uint8_t Channels)
{
	/*检查参数: 注意这里的通道可以存在 或 "|" 关系, 可以同时配置多个通道*/
	if(xId > TIM_ID_MAX){
		return R_PARAM;
	}
	
	/*base init*/
	if(!_timer_init_state[xId]){
		TIM_HandleTypeDef  *htim_temp     = &htim[xId];
		htim_temp->Instance               = TIM_CFG[xId].BASE;              /*设备及基地址*/
		htim_temp->Init.Prescaler         = psc;             				/*预分频系数*/
		htim_temp->Init.CounterMode       = TIM_COUNTERMODE_UP;            	/*计数模式:向上计数*/
		htim_temp->Init.Period            = arr;        					/*自动装载值*/
		htim_temp->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;       	/*定时器时钟分频*/
	//    htim_temp->Init.RepetitionCounter =                               /*重复计数器*/
		htim_temp->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;	/*自动重装载*/
		HAL_TIM_PWM_Init(&htim[xId]); 
		
		/*AFIO init*/
		switch(xId){
			case TIM_ID1: __TIM1_AFIO(); break;
			case TIM_ID2: __TIM2_AFIO(); break;
			case TIM_ID3: __TIM3_AFIO(); break;
			case TIM_ID4: __TIM4_AFIO(); break;
			default: break;
		}
		
		_timer_init_state[xId] = true;
	}
	
    /*chn init*/
    TIM_OC_InitTypeDef oc_init_temp = {0};
    oc_init_temp.OCMode             = TIM_OCMODE_PWM1;                          /*输出比较模式： PWM1*/   
    oc_init_temp.Pulse              = 0;                                        /*默认不输出，0%占空比*/    
    oc_init_temp.OCPolarity         = TIM_OCPOLARITY_HIGH;                       /*输出比较极性为低*/
//    oc_init_temp.OCNPolarity        = TIM_OCPOLARITY_HIGH;                      /*互补通道比较极性为高*/
//    oc_init_temp.OCFastMode         = TIM_OCFAST_DISABLE;                       /*失能快速模式(仅PWM1/PWM2模式有效)*/
//    oc_init_temp.OCIdleState        = TIM_OCIDLESTATE_SET;                      /*空闲状态电平*/
//    oc_init_temp.OCNIdleState       = TIM_OCIDLESTATE_RESET;                    /*互补通道空闲状态电平*/

	if(Channels & TIM_CHN1){
		HAL_TIM_PWM_ConfigChannel(&htim[xId], &oc_init_temp, TIM_CHANNEL_1);        
		HAL_TIM_PWM_Start(&htim[xId], TIM_CHANNEL_1);	
		my_gpio.init(TIM_CFG[xId].Chn[0].GPIOx, TIM_CFG[xId].Chn[0].GPIO_Pin, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);/*gpio init*/
	}
	
	if(Channels & TIM_CHN2){
		HAL_TIM_PWM_ConfigChannel(&htim[xId], &oc_init_temp, TIM_CHANNEL_2);        
		HAL_TIM_PWM_Start(&htim[xId], TIM_CHANNEL_2);	
		my_gpio.init(TIM_CFG[xId].Chn[1].GPIOx, TIM_CFG[xId].Chn[1].GPIO_Pin, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);/*gpio init*/
	}
	
	if(Channels & TIM_CHN3){
		HAL_TIM_PWM_ConfigChannel(&htim[xId], &oc_init_temp, TIM_CHANNEL_3);        
		HAL_TIM_PWM_Start(&htim[xId], TIM_CHANNEL_3);	
		my_gpio.init(TIM_CFG[xId].Chn[2].GPIOx, TIM_CFG[xId].Chn[2].GPIO_Pin, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);/*gpio init*/
	}

	if(Channels & TIM_CHN4){
		HAL_TIM_PWM_ConfigChannel(&htim[xId], &oc_init_temp, TIM_CHANNEL_4);        
		HAL_TIM_PWM_Start(&htim[xId], TIM_CHANNEL_4);	
		my_gpio.init(TIM_CFG[xId].Chn[3].GPIOx, TIM_CFG[xId].Chn[3].GPIO_Pin, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);/*gpio init*/
	}

    return R_OK;
}

/** 
 *@brief 定时器PWM输出配置
 */
static int my_timer_pwm_set(my_timer_id xId, uint8_t Channels, uint32_t DutyCycle)
{
    /*检查参数: 注意这里的通道不能存在 或 "|" 关系, 一次只能配置一个通道的占空比*/
	if(xId > TIM_ID_MAX){
		return R_PARAM;
	}
		
    /*检查定时器初始化状态*/

    /*检查通道初始化状态*/

    /*比较值不可比重转载值大*/

    /*设置重装载值*/
	if(Channels & TIM_CHN1) __HAL_TIM_SET_COMPARE(&htim[xId], TIM_CHANNEL_1, DutyCycle);
	if(Channels & TIM_CHN2)	__HAL_TIM_SET_COMPARE(&htim[xId], TIM_CHANNEL_2, DutyCycle); 
	if(Channels & TIM_CHN3)	__HAL_TIM_SET_COMPARE(&htim[xId], TIM_CHANNEL_3, DutyCycle); 
	if(Channels & TIM_CHN4)	__HAL_TIM_SET_COMPARE(&htim[xId], TIM_CHANNEL_4, DutyCycle); 
	
    return R_OK;  	
}
	
/** 
 *@brief 定时器编码器模式初始化
 */
static int my_timer_encoder_init(my_timer_id xId, uint32_t Channel)
{
	
}

/** 
 *@brief 定时器ETR外部时钟源模式初始化
 */
static int my_timer_etr_init(my_timer_id xId, uint32_t Channel)
{
	
}

/** 
 *@brief 定时器IC输入捕获模式初始化
 */
static int my_timer_ic_init(my_timer_id xId, uint32_t Channel)
{
	
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
* @note
* HAL_TIM_Base_Start_IT(&htim1); //使用定时器的时候调用这个函数启动
* HAL_TIM_Base_Stop_IT(&htim1);  //停止定时器的时候调用这个函数关闭

* @note
* 通用定时器(2,3,4)的时钟来自 APB1,当 PPRE1 ≥ 2 分频的时候
* 通用定时器的时钟为 APB1 时钟的 2 倍, 而 APB1 为 36M, 所以定时器时钟=72Mhz
* 定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
* Ft=定时器工作频率,单位:Mhz

/*encoder*/
/*
定时器 编码器模式下，在上升沿和下降沿均会计数（不可配置），但是可以选择只计一个通道(原始信号两倍频), 但是可以选择只计一个通道(原始信号两倍频), 

1. EncoderMode：编码器模式选择，用来设置计数器采集编码器信号的方式，可选通道 A 计
数、通道 B 计数和双通道计数。它设定 TIMx_SMCR 寄存器的 SMS[2:0] 位。这个成员实际
是用来设置编码器接口的倍频数的，当选择通道 A 或 B 计数时为 2 倍频，双通道计数时为4 倍频。

2. ICxPolarity：输入捕获信号极性选择，用于设置定时器通道在编码器模式下的输入信号是否反相。它设定 TIMx_CCER 寄存器的 CCxNP 位和 CCxP 位。

3. ICxSelection： 输入通道选择，ICx的信号可来自三个输入通道， 分别为 IM_ICSELECTION_DIRECTTI、 
TIM_ICSELECTION_INDIRECTTI或IM_ICSELECTION_TRC。它设定 TIMx_CCMRx 寄存器的 CCxS[1:0] 位的值。定时器在编码器接口模式下，此成员只能设置为 TIM_ICSELECTION_DIRECTTI。

4. ICxPrescaler：输入捕获通道预分频器，可设置 1、 2、 4、 8 分频。它设定 TIMx_CCMRx 寄存器的 ICxPSC[1:0] 位的值。

5. ICxFilter：输入捕获滤波器设置，可选设置 0x0 至 0x0F。它设定 TIMx_CCMRx 寄存器ICxF[3:0] 位的值。
*/


