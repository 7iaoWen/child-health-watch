/********************************************************************************
* @file		:my_exti.c
* @brief	:对STM32 HAL 库中 EXTI 外部中断的二次封装 (依赖于 my_systick 中的软件定时器)
* @Author	:Gwen9
* @Date		:2024/08/01
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#include "onchip_conf.h"

#ifdef ONCHIP_MY_EXTI_ENABLED

/* Private Macros ---------------------------------------------------------------------*/
#define MY_EXTI_LINE_COUNT	16	//外部中断线数量


/* Private Typedef  -------------------------------------------------------------------*/
/** 
  * @brief  EXTI LINE 状态枚举类型
  */
typedef enum
{ 
	MY_EXTI_IDEL = 0,		//中断线: 空闲
	MY_EXTI_TRIG_FIRST,		//中断线: 首次触发
	MY_EXTI_TRIG_CONFIRM,	//中断线: 确认触发（消抖后）
	EXTI_TRIG_LONG,		    //中断线: 长时间触发（大于等于 EXTI_TRIG_LAST_TIME）
	MY_EXTI_TRIG_WAIT,		//中断线: 等待恢复空闲状态
}my_exti_state_t;

/** 
 *@brief EXTI 对象私有成员结构体 
 */
typedef struct _S_MY_EXTI
{
	bool				init_state	;
	my_exti_state_t		state		;
	GPIO_TypeDef* 		GPIOx		;
	uint16_t 			GPIO_Pin	;
	uint8_t 			polarity	;
	uint32_t			trig_tick	;	//外部中断的触发时间
	uint32_t 			trig_count	;	//外部中断的触发次数
	const char*   		name    	;   //按键名称
}s_my_exti;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_exti_init(exti_list_t* list, uint8_t list_len, int (*callback)(const char*, uint8_t));
static int my_exti_get_trig_count(const char* name, uint32_t *trig_count, bool clear);
static void timer_callback(void *param);
static int gpio_pin_to_number(uint16_t gpio_pin);

/* Private Variables -------------------------------------------------------------------*/
//参数定义: EXTI 单例模型，扩展到外部使用定义
const c_my_exti my_exti = {my_exti_init, my_exti_get_trig_count};
//参数定义: EXTI 外部中断 初始化状态
static bool my_exti_init_state = false;		
//参数定义: EXTI 外部中断 用到的定时器 ID
static int  exti_timer_id;			
//参数定义: EXTI 外部中断 私有成员参数
static s_my_exti g_my_exti[MY_EXTI_LINE_COUNT];
//参数定义: EXTI 外部中断 回调函数
static int (*my_exti_callback)(const char*, uint8_t);

/* Public Functions -------------------------------------------------------------------*/
static int my_exti_init(exti_list_t* list, uint8_t list_len, int (*callback)(const char*, uint8_t))
{
	GPIO_InitTypeDef GPIO_InitStruct;		/*GPIO初始化结构体*/	
	int pin_num;

	/*如果已经初始化了这里直接返回*/
	if(my_exti_init_state != false){
		return R_OK;
	}

	/*初始化 EXTI 对象用到的定时器*/
	exti_timer_id = my_systick.create_timer(20, timer_callback, NULL); 	//创建软件定时器
	if(exti_timer_id < 0)		return R_ERROR;							//定时器创建失败
	
	/*初始化 EXTI 对象的私有成员*/
	for(uint8_t i = 0; i < list_len; i++){
		pin_num = gpio_pin_to_number(list[i].GPIO_Pin);	//获取引脚对应的中断线序号
		if(pin_num < 0 || pin_num > MY_EXTI_LINE_COUNT)	return R_ERROR;	//引脚参数错误
		/*初始化*/
		if(!g_my_exti[pin_num].init_state){				 
			if(list[i].Polarity == MY_EXTI_FALLING){	 /*下降沿触发，空闲状态为高电平*/
				my_gpio.init(list[i].GPIOx, list[i].GPIO_Pin, GPIO_MODE_IT_FALLING, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
			}else{										 /*上升沿触发，空闲状态为低电平*/
				my_gpio.init(list[i].GPIOx, list[i].GPIO_Pin, GPIO_MODE_IT_RISING, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH);
			}
			/*EXTI 的 NVIC 中断控制器 已经 在 HAL_MspInit 中全部初始化，这里无需再初始化*/
			g_my_exti[pin_num].state 	  	= MY_EXTI_IDEL;
			g_my_exti[pin_num].name			= list[i].name;
			g_my_exti[pin_num].GPIOx 		= list[i].GPIOx;
			g_my_exti[pin_num].GPIO_Pin 	= list[i].GPIO_Pin;
			g_my_exti[pin_num].init_state 	= true;
		}
	}
	/*直接启动定时器*/
	my_systick.start_timer(exti_timer_id);								
	/*初始化 外部中断回调函数*/
	my_exti_callback = callback;
	/*设置初始化状态*/
	my_exti_init_state = true;

	return R_OK;
}

static int my_exti_get_trig_count(const char* name, uint32_t *trig_count, bool clear)
{
	/*参数检测*/
	if(name == NULL || trig_count == NULL){
		return R_NULL;
	}

	for(uint8_t i = 0; i < MY_EXTI_LINE_COUNT; i++){
		/*外部中断线未初始化，直接跳过*/
		if(!g_my_exti[i].init_state)	continue;	
		/*找到对应的 外部中断线名称*/
		if(strcmp(name, g_my_exti[i].name) == 0){
			*trig_count = g_my_exti[i].trig_count;		//返回计数
			if(clear)	g_my_exti[i].trig_count = 0;	//清空计数
			return R_OK;
		}
	}
	return R_OK;
}
/* Private Functions -------------------------------------------------------------------*/
static int gpio_pin_to_number(uint16_t gpio_pin)
{
    int pin_number = 0;
    // 如果输入为0，则返回-1，表示无效
    if(gpio_pin == 0)	return -1;
    // 循环找到设置为1的位，并返回对应的引脚编号
    while (gpio_pin != 0x0001){
        gpio_pin >>= 1;  // 右移1位
        pin_number++;    // 计数加1
    }
    return pin_number;
}

static void timer_callback(void *param)
{
	int ret;

	GPIO_PinState bitstatus;
	uint32_t curtick = HAL_GetTick();		//获取当前时间

	for(uint8_t i = 0; i < MY_EXTI_LINE_COUNT; i++){
		/*外部中断线未初始化 或者 外部中断线 为 空闲状态，直接跳过*/
		if(!g_my_exti[i].init_state || g_my_exti[i].state == MY_EXTI_IDEL)	continue;	
		/*当前时间 未超过消抖时间 直接跳过*/
		if(curtick <= g_my_exti[i].trig_tick + 20)		continue;

		/*获取IO口状态*/
		bitstatus = HAL_GPIO_ReadPin(g_my_exti[i].GPIOx, g_my_exti[i].GPIO_Pin);
		
		if(g_my_exti[i].state == MY_EXTI_TRIG_FIRST){	
			if(g_my_exti[i].polarity ^ bitstatus){					//检测信号（异或）：下降沿0 且 为高电平1 / 上升沿1 且为 低电平0 时 表示触发结束
				g_my_exti[i].state = MY_EXTI_IDEL;					//恢复空闲状态
			}else{													//否则认为消抖成功：下降沿0 且 为低电平0 / 上升沿1 且为 高电平1 时	
				g_my_exti[i].state = MY_EXTI_TRIG_CONFIRM;
				my_exti_callback(g_my_exti[i].name, EXTI_TRIG);
			}			
		}else if(g_my_exti[i].state == MY_EXTI_TRIG_CONFIRM){
			if(g_my_exti[i].polarity ^ bitstatus){					//检测信号（异或）：下降沿0 且 为高电平1 / 上升沿1 且为 低电平0 时 表示触发结束
				g_my_exti[i].state = MY_EXTI_TRIG_WAIT;				//进入等待状态
			}else if(curtick >= g_my_exti[i].trig_tick + EXTI_TRIG_LAST_TIME){				//否则判断是否 属于长时间触发 
				my_exti_callback(g_my_exti[i].name, EXTI_TRIG_LAST);//调用回调函数	
				g_my_exti[i].state = MY_EXTI_TRIG_WAIT;				//进入等待状态				
			}
		}else if(g_my_exti[i].state == MY_EXTI_TRIG_WAIT){
			if(g_my_exti[i].polarity ^ bitstatus){					//检测信号（异或）：下降沿0 且 为高电平1 / 上升沿1 且为 低电平0 时 表示触发结束
				my_exti_callback(g_my_exti[i].name, EXTI_TRIG_CLEAR);
				g_my_exti[i].state = MY_EXTI_IDEL;					//恢复空闲状态
			}
		}
	}
}

/*外部中断回调函数*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	/*获取引脚对应的中断线序号*/
	int pin_num = gpio_pin_to_number(GPIO_Pin);
	if(pin_num < 0 || pin_num > MY_EXTI_LINE_COUNT)		return;
	/*外部中断线未初始化 或者 外部中断线非空闲状态，直接返回*/
	if(!g_my_exti[pin_num].init_state || g_my_exti[pin_num].state != MY_EXTI_IDEL)	return;
	/*改变外部中断线状态，记录触发时间，触发计数次数加一*/
	g_my_exti[pin_num].state = MY_EXTI_TRIG_FIRST;
	g_my_exti[pin_num].trig_tick = HAL_GetTick();
	g_my_exti[pin_num].trig_count++;
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5))		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6)) 		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7))		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8)) 		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9))		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10)) 	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
}

void EXTI15_10_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15))	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
