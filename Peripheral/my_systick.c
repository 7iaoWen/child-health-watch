/********************************************************************************
* @file		:my_systick.c
* @brief	:对STM32 HAL 库中 SYSTICK 外设进行的二次封装，实现了 延时函数/软件定时器等功能
* @Author	:Gwen9
* @Date		:2024/08/06
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_SYSTICK_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/
/*软件定时器*/
typedef struct{
	bool		exist;					//存在标志
	bool 		active; 				//启用标注
    uint32_t 	period;         		// Timer period (in ms)
    uint32_t 	counter;        		// Current counter value (in ms)
	void* 		param;
    void(*callback)(void *param);  		
} my_software_timer_t;

static my_software_timer_t s_theTimer[MAX_TIMERS];

/* Private Functions Declare ----------------------------------------------------------*/
static int my_systick_init(uint32_t ticks, uint32_t TickPriority);
static void my_systick_delay_us(uint32_t nus);
static void my_systick_delay_ms(uint32_t nms);
static int my_systick_create_timer(uint32_t period, void (*callback)(void *param), void *param);
static int my_systick_delete_timer(int timer_id);
static int my_systick_start_timer(int timer_id);
static int my_systick_stop_timer(int timer_id);
static int my_systick_change_period(int timer_id, uint32_t period);

static int my_systick_period_task_running(void);
static int my_systick_period_task_register(my_systick_task_period_t period, void (*callback)(void *), void *param);
static bool my_systick_period_state_get(my_systick_task_period_t period);
static void my_systick_period_state_clear(my_systick_task_period_t period);
/* Private Variables -------------------------------------------------------------------*/
//参数定义:	SYSTICK 单例模型，外部接口定义
const c_my_systick my_systick = {	my_systick_init, 				my_systick_delay_us, 				my_systick_delay_ms,
									my_systick_create_timer, 		my_systick_delete_timer, 			my_systick_start_timer,			
									my_systick_stop_timer, 			my_systick_change_period,
									my_systick_period_task_running, my_systick_period_task_register};

//参数定义:	周期事件标志位
static volatile bool period_state[MY_SYSTICK_PERIOD_TYPE_MAX] = {false};

//参数定义:	定义函数 指针列表， 产生不同周期时间中断时会调用列表中对应的函数
static void(*_1ms_task_list		[MY_SYSTICK_10MS_TASK_MAX]	)(void *);
static void(*_10ms_task_list		[MY_SYSTICK_10MS_TASK_MAX]	)(void *);
static void(*_100ms_task_list		[MY_SYSTICK_100MS_TASK_MAX]	)(void *);
static void(*_1000ms_task_list		[MY_SYSTICK_1000MS_TASK_MAX])(void *);

//参数定义:	定义函数 参数列表，调用回调函数时会传入以下参数
static void* _1ms_task_param_list		[MY_SYSTICK_1MS_TASK_MAX]	;
static void* _10ms_task_param_list		[MY_SYSTICK_10MS_TASK_MAX]	;
static void* _100ms_task_param_list	[MY_SYSTICK_100MS_TASK_MAX]	;
static void* _1000ms_task_param_list	[MY_SYSTICK_1000MS_TASK_MAX];

//参数定义:	中断任务启动状态
static bool my_systick_interrupt_task_state;

/* Public Functions -------------------------------------------------------------------*/
/** 
 *@brief SYSTICK 初始化函数
 *注：HAL 中一般会自动调用 SYSTICK 初始化函数，所以这里一般不需要再次调用，直接使用即可
 *调用路径如下： HAL_Init -> HAL_InitTick -> HAL_SYSTICK_Config -> SysTick_Config
 */
static int my_systick_init(uint32_t ticks, uint32_t TickPriority)
{
	//ticks 为设置的重装载值，这里需要检测 参数是否超出范围 最大值为 0xFFFFFF 24bits
	if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk)
	{
		return (1UL);                                                 
	}

	//设置重装载寄存器, 设置后 VAL 寄存器从设置的值开始向下递减计数，计数到零时产生 SysTick_Handler 中断
	SysTick->LOAD  = (uint32_t)(ticks - 1UL);  

	//设置中断优先级，这里设优先级为允许的最大值，即最低优先级
	NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); 
	
	//设置当前数值寄存器为零
	SysTick->VAL   = 0UL; 

	//通过 CTRL 控制寄存器 配置 SysTick 相关参数
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |	// 设置系统定时器的时钟源为 AHBCLK = 72M
				   SysTick_CTRL_TICKINT_Msk   |		// 使能系统定时器中断
				   SysTick_CTRL_ENABLE_Msk;         // 使能定时器 
	
	//配置 SysTick 的中断优先级 此函数在 HAL_InitTick 被调用			   
	/* Configure the SysTick IRQ priority */
	if(TickPriority < (1UL << __NVIC_PRIO_BITS))
	{
		HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
		uwTickPrio = TickPriority;
	}
	else
	{
		return R_ERROR;
	}
  
	return R_OK; 
}	

static void my_systick_delay_us(uint32_t nus)
{
	uint32_t ticks;
	uint32_t told, tnow, tcnt = 0;
	uint32_t reload = SysTick->LOAD;	//重装载值	
	
	if(0 == nus)	return;
	
	/*计算需要的节拍数
	-SysTick 的时钟源初始化为 AHBCLK = 72M
	-对应 计数器每减一次的时间为 1/72000000 
	-对应 每 us 需要递减的节拍数为 _TICK_US = 72
    -对饮 nus 需要递减的节拍数为 _TICK_US *nus = ticks */
	ticks = nus * _TICK_US;

	/*刚进入时的计数器值*/	
	told = SysTick->VAL;        				
    
	/*时间积分过程，直到循环的时间累加到等于 ticks 时退出
	-通过 tnow 和 told 计算每次循环的时间
	-将每次循环的时间累加到 tcnt 上
	-当 tcnt = ticks 时退出基本上刚好等于需要延时的时间*/
	while(1){
		tnow = SysTick->VAL;	
		if(tnow != told){	//检测到时间变化						    
			if(tnow < told)	tcnt += told - tnow;		 //SYSTICK是一个递减的计数器，这里没有溢出，直接相减即可得到两次循环的间隔时间，然后累加即可
			else 			tcnt += reload - tnow + told;//这里溢出了，需要通过重装载值计算两次循环的间隔时间	，然后累加即可    
			told = tnow;
			if(tcnt >= ticks)	break;					 //时间超过/等于要延迟的时间,则退出.
		}  
	};
}

static void my_systick_delay_ms(uint32_t nms)
{
	//延时nms,不会引起任务调度,阻塞延时
	uint32_t i;
	for(i = 0; i < nms; i++){
		my_systick_delay_us(1000);
	} 
}

static int my_systick_period_task_running(void)
{
	if(period_state[MY_SYSTICK_PERIOD_1MS]){
		for(uint8_t i = 0; i < MY_SYSTICK_1MS_TASK_MAX; i++){
			if(_1ms_task_list[i] != NULL){
				_1ms_task_list[i](_1ms_task_param_list[i]);
			}
		}
		period_state[MY_SYSTICK_PERIOD_1MS] = false;
	}

	if(period_state[MY_SYSTICK_PERIOD_10MS]){
		for(uint8_t i = 0; i < MY_SYSTICK_10MS_TASK_MAX; i++){
			if(_10ms_task_list[i] != NULL){
				_10ms_task_list[i](_10ms_task_param_list[i]);
			}
		}
		period_state[MY_SYSTICK_PERIOD_10MS] = false;
	}

	if(period_state[MY_SYSTICK_PERIOD_100MS]){
		for(uint8_t i = 0; i < MY_SYSTICK_100MS_TASK_MAX; i++){
			if(_100ms_task_list[i] != NULL){
				_100ms_task_list[i](_100ms_task_param_list[i]);
			}
		}	
		period_state[MY_SYSTICK_PERIOD_100MS] = false;
	}

	if(period_state[MY_SYSTICK_PERIOD_1000MS]){
		for(uint8_t i = 0; i < MY_SYSTICK_1000MS_TASK_MAX; i++){
			if(_1000ms_task_list[i] != NULL){
				_1000ms_task_list[i](_1000ms_task_param_list[i]);
			}
		}
		period_state[MY_SYSTICK_PERIOD_1000MS] = false;
	}
	return R_OK;
}

static int my_systick_period_task_register(my_systick_task_period_t period, void (*callback)(void *), void *param)
{
	uint8_t i;
	if(callback == NULL){
		return R_NULL;	
	}	

	switch (period)
	{
	case MY_SYSTICK_PERIOD_1MS:
		for(i = 0; i < MY_SYSTICK_1MS_TASK_MAX; i++){
			if(_1ms_task_list[i] == NULL){
				_1ms_task_list[i] = callback;
				_1ms_task_param_list[i] = param;
				break;
			}
		}
		if(i == MY_SYSTICK_1MS_TASK_MAX)	return R_ERROR;	break;
	case MY_SYSTICK_PERIOD_10MS:
		for(i = 0; i < MY_SYSTICK_10MS_TASK_MAX; i++){
			if(_10ms_task_list[i] == NULL){
				_10ms_task_list[i] = callback;
				_10ms_task_param_list[i] = param;
				break;
			}
		}
		if(i == MY_SYSTICK_10MS_TASK_MAX)	return R_ERROR; break;
	case MY_SYSTICK_PERIOD_100MS:
		for(i = 0; i < MY_SYSTICK_100MS_TASK_MAX; i++){
			if(_100ms_task_list[i] == NULL){
				_100ms_task_list[i] = callback;
				_100ms_task_param_list[i] = param;
				break;
			}
		}
		if(i == MY_SYSTICK_100MS_TASK_MAX)	return R_ERROR; break;
	case MY_SYSTICK_PERIOD_1000MS:
		for(i = 0; i < MY_SYSTICK_1000MS_TASK_MAX; i++){
			if(_1000ms_task_list[i] == NULL){
				_1000ms_task_list[i] = callback;
				_1000ms_task_param_list[i] = param;
				break;
			}
		}
		if(i == MY_SYSTICK_1000MS_TASK_MAX)	return R_ERROR; break;
	default:
		break;
	}


	return R_OK;
}

static int my_systick_create_timer(uint32_t period, void (*callback)(void *param), void *param)
{
	for(int i = 0; i < MAX_TIMERS; i++) {
        if (!s_theTimer[i].exist) {
            s_theTimer[i].period 	= period;		//定时器周期
            s_theTimer[i].counter 	= period;		//装载计数周期
            s_theTimer[i].callback 	= callback;		//设置回调函数
			s_theTimer[i].param		= param;		//回调函数参数
            s_theTimer[i].exist 	= true;			//标志定时器创建成功
			s_theTimer[i].active 	= false;		//标志定时器尚未启动	
            return i; 								// Return the timer ID
        }
    }
    return -1; 			// No available timer slot
}

static int my_systick_delete_timer(int timer_id)
{
	/*timer_id 正确且已经创建 的定时器才可以删除*/
    if (timer_id >= 0 && timer_id < MAX_TIMERS && s_theTimer[timer_id].exist){
        s_theTimer[timer_id].exist 		= false;
        s_theTimer[timer_id].active 	= false;
		return R_OK;
    }	
	return R_ERROR;
}
	
static int my_systick_start_timer(int timer_id)
{
	/*timer_id 正确且已经创建 的定时器才可以启动*/
    if (timer_id >= 0 && timer_id < MAX_TIMERS && s_theTimer[timer_id].exist){
        s_theTimer[timer_id].counter = s_theTimer[timer_id].period;
        s_theTimer[timer_id].active = true;
		return R_OK;
    }	
	return R_ERROR;
}

static int my_systick_stop_timer(int timer_id)
{
	/*timer_id 正确且已经创建 的定时器才可以停止*/
    if (timer_id >= 0 && timer_id < MAX_TIMERS && s_theTimer[timer_id].exist){
        s_theTimer[timer_id].active = false;
		return R_OK;
    }
	return R_ERROR;
}

static int my_systick_change_period(int timer_id, uint32_t period)
{
	/*timer_id 正确且已经创建 的定时器才修改周期时间*/
    if (timer_id >= 0 && timer_id < MAX_TIMERS && s_theTimer[timer_id].exist){
		s_theTimer[timer_id].period 	= period;
		s_theTimer[timer_id].counter 	= period;
		return R_OK;
    }
	return R_ERROR;
}
	
void HAL_SYSTICK_Callback(void) 
{
	uint32_t sys_cnt = HAL_GetTick();
	period_state[MY_SYSTICK_PERIOD_1MS] = true;
	if((sys_cnt%10) == 0)		period_state[MY_SYSTICK_PERIOD_10MS] 	= true;
	if((sys_cnt%100) == 0)		period_state[MY_SYSTICK_PERIOD_100MS] 	= true;
	if((sys_cnt%1000) == 0)		period_state[MY_SYSTICK_PERIOD_1000MS] 	= true;
    for(int i = 0; i < MAX_TIMERS; i++){
        if(s_theTimer[i].active){
            if(s_theTimer[i].counter > 0){
                s_theTimer[i].counter--;
                if(s_theTimer[i].counter == 0){
                    s_theTimer[i].callback(s_theTimer[i].param); 		// Call the callback function
                    s_theTimer[i].counter = s_theTimer[i].period; 		// Reset the counter
                }
            }
        }
    }
}
/* Private Functions -------------------------------------------------------------------*/

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/*
1.SysTick 简介
 -SysTick—系统定时器是属于 CM3 内核中的一个外设，内嵌在 NVIC 中。系统定时器是一个 24bit 的向下递减的计数器，
 -计数器每计数一次的时间为 1/SYSCLK，一般我们设置系统时钟 SYSCLK 等于 72M。
 -当重装载数值寄存器的值递减到 0 的时候，系统定时器就产生一次中断，以此循环往复。
 -因为 SysTick 是属于 CM3 内核的外设，所以所有基于 CM3 内核的单片机都具有这个系统定时器，
 -使得软件在 CM3 单片机中可以很容易的移植。系统定时器一般用于操作系统，用于产生时基，维持操作系统的心跳。

2. SysTick 寄存器介绍
 -SysTick—系统定时器有 4 个寄存器，简要介绍如下。在使用 SysTick 产生定时的时候，只需要配置前三个寄存器，最后一个校准寄存器不需要使用。
 
 -CTRL 	 控制及状态寄存器
 -LOAD 	 重装载数值寄存器
 -VAL 	 当前数值寄存器
 -CALIB  校准数值寄存器

3. HAL中自带 SysTick 初始化函数 SysTick_Config 分析
 -函数调用路径： HAL_Init -> HAL_InitTick -> HAL_SYSTICK_Config -> SysTick_Config
 -SysTick_Config 函数说明
 
__STATIC_INLINE uint32_t SysTick_Config(uint32_t ticks)
{
	//ticks 为设置的重装载值，这里需要检测 参数是否超出范围 最大值为 0xFFFFFF 24bits
	if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk)
	{
		return (1UL);                                                 
	}

	//设置重装载寄存器, 设置后 VAL 寄存器从设置的值开始向下递减计数，计数到零时产生 SysTick_Handler 中断
	SysTick->LOAD  = (uint32_t)(ticks - 1UL);  

	//设置中断优先级，这里设优先级为允许的最大值，即最低优先级
	NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); 
	
	//设置当前数值寄存器为零
	SysTick->VAL   = 0UL; 

	//通过 CTRL 控制寄存器 配置 SysTick 相关参数
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |	// 设置系统定时器的时钟源为 AHBCLK=72M
				   SysTick_CTRL_TICKINT_Msk   |		// 使能系统定时器中断
				   SysTick_CTRL_ENABLE_Msk;         // 使能定时器 	      
				   
	return (0UL);                                                 
}

4.基于 SysTick 的自定义软件定时器和 FreeRTOS 中软件定时器的对比
--相似之处
 -基于“节拍”计时：
	两者都使用系统节拍（tick）来进行时间计量。在你的实现中，SysTick定时器用于产生每毫秒一次的中断；在FreeRTOS中，系统节拍通常由一个硬件定时器（如SysTick）产生。
 -计时器回调：
	两者都有一个回调函数，当计时器到期时调用。你的实现和FreeRTOS的软件定时器都可以设置定时器的周期和回调函数。
	
--区别
 -定时器的管理和调度：
	我们的实现	：在SysTick中断处理程序中，遍历所有软件定时器并减少其计数值。到期时调用回调函数，并重置计数值。
	FreeRTOS	：FreeRTOS有一个专用的定时器任务（Timer Daemon Task），所有软件定时器的回调函数都在这个任务的上下文中执行。这允许定时器的回调函数与其他任务分离，并提供更灵活的管理。
 -精确度和性能：
	我们的实现	：所有的定时器都在SysTick中断处理程序中处理，这可能会导致在计时器数量较多时，SysTick中断处理程序变得很重，影响系统的实时性能。
	FreeRTOS	：FreeRTOS的软件定时器处理在一个独立的任务中进行，这样可以避免中断处理程序中执行大量代码，提高系统的实时性。
 -功能和灵活性：
	我们的实现	：提供了基本的定时器创建、启动和停止功能。
	FreeRTOS	：FreeRTOS的软件定时器提供了更多的功能和灵活性，包括一次性定时器和周期性定时器、动态创建和删除定时器，以及定时器的暂停和恢复。
 -系统负载：
	我们的实现	：所有的计时器回调都在中断上下文中运行，这可能会影响系统的整体性能，尤其是在回调函数执行时间较长的情况下。
	FreeRTOS	：定时器回调在定时器任务的上下文中执行，允许其他中断和任务继续运行，提高了系统的响应性和负载管理。
*/



