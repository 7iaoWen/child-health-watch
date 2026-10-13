/********************************************************************************
* @file		:my_systick.h
* @brief	:my_systick.c头文件
* @Author	:Gwen9
* @Date		:2024/08/06
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_SYSTICK_H__
#define __MY_SYSTICK_H__

/* Public Macros  -------------------------------------------------------------------*/
#define MY_SYSTICK_1MS_TASK_MAX         1
#define MY_SYSTICK_10MS_TASK_MAX        5
#define MY_SYSTICK_100MS_TASK_MAX       5
#define MY_SYSTICK_1000MS_TASK_MAX      5
#define MY_SYSTICK_10S_TASK_MAX         5

/* Public Typedef  -------------------------------------------------------------------*/
typedef enum
{
    MY_SYSTICK_PERIOD_1MS = 0,
    MY_SYSTICK_PERIOD_10MS,
    MY_SYSTICK_PERIOD_100MS,
    MY_SYSTICK_PERIOD_1000MS,
    MY_SYSTICK_PERIOD_TYPE_MAX,
}my_systick_task_period_t;

/** 
  * @brief  GPIO 初始化类型，输入/输出 
  */
typedef struct _MY_SYSTICK
{
    /************************************************* 
    * Description: SYSTICK 初始化函数(注：一般无需调用)
	*			HAL 中一般会自动调用 SYSTICK 初始化函数，所以这里一般不需要再次调用，直接使用即可
	*			调用路径如下： HAL_Init -> HAL_InitTick -> HAL_SYSTICK_Config -> SysTick_Config
    * Input : <ticks> SYSTICK 自动重装载值;
			  <TickPriority> SYSTICK 中断优先级;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			 ret = my_systick.init();
    *************************************************/  
	int(*init)(uint32_t ticks, uint32_t TickPriority);

	/************************************************* 
    * Description: 阻塞延时 n 微秒
    * Input : <nus> 延时时间,单位 us;
    * Return: 无
    * Demo  : 
			 my_systick.delay_us(100);
    *************************************************/ 
	void(*delay_us)(uint32_t nus);

	/************************************************* 
    * Description: 阻塞延时 n 毫秒
    * Input : <nms> 延时时间,单位 ms;
    * Return: 无
    * Demo  : my_systick.delay_ms(100);
    *************************************************/ 
	void(*delay_ms)(uint32_t nms);
	
	/************************************************* 
    * Description: 创建一个软件定时器
    * Input : <period> 延时时间,单位 ms;
			  <callback> 软件定时器回调函数;
			  <param> 	软件定时器回调函数参数;
	* Return: 0-MAX_TIMERS: 软件定时器ID
			  -1:创建失败
    * Demo  : 
			void timer_callback(void* param)
			{
				;
			}
			int timer_id;
			uint8_t data[8];
			timer_id = my_systick.create_timer(100，timer_callback, (void*)data);
    *************************************************/ 
	int(*create_timer)(uint32_t period, void (*callback)(void *param), void *param);
	
	/************************************************* 
    * Description: 删除指定软件定时器
    * Input : <timer_id> 定时器ID;
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  my_systick.delete_timer(timer_id);
    *************************************************/ 
	int(*delete_timer)(int timer_id);
	
	/************************************************* 
    * Description: 启动指定软件定时器
    * Input : <timer_id> 定时器ID;
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  my_systick.start_timer(timer_id);
    *************************************************/ 
	int(*start_timer)(int timer_id);
	
	/************************************************* 
    * Description: 停止指定软件定时器
    * Input : <timer_id> 定时器ID;
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  my_systick.stop_timer(timer_id);
    *************************************************/ 
	int(*stop_timer)(int timer_id);
	
	/************************************************* 
    * Description: 改变软件定时器定时周期
    * Input : <timer_id> 定时器ID;
			  <period> 需要重设的周期时间;
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  my_systick.period_timer(timer_id, 500);
    *************************************************/ 
	int(*period_timer)(int timer_id, uint32_t period);

	/************************************************* 
    * Description: 运行周期定时器任务，直接在主函数 while 中调用
    *              会自动执行已经注册的 周期回调函数，一般 底层驱动会注册此类函数
    *              应用层开发无需关心此函数具体实现 直接在主函数 while 中调用此函数即可
    * Input : 无
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  my_systick.period_task_running();
    *************************************************/ 
	int(*period_task_running)(void);

	/************************************************* 
    * Description: 注册周期定时函数，注册后的函数，在调用 period_task_running 函数时会执行
    * Input : <period>  函数执行周期，枚举类型 my_systick_task_period_t， 1ms - 10s 可选;
			  <callback> 需要执行的函数指针;
              <param>   函数参数;
	* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
    *         void period_func(void *param){ };
			  my_systick.period_task_register(MY_SYSTICK_PERIOD_10MS, period_func, param);
    *************************************************/ 
	int(*period_task_register)(my_systick_task_period_t period, void (*callback)(void *), void *param);
}c_my_systick;

/** 
  * @brief  片上 SYSTICK 外设, 单例模型，扩展到外部使用
  */
extern const c_my_systick my_systick;


#endif 

