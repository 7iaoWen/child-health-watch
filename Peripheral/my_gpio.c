/********************************************************************************
* @file		:my_gpio.c
* @brief	:对STM32 HAL 库中 GPIO 的二次封装，提供GPIO的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/01
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#include "onchip_conf.h"

#ifdef ONCHIP_MY_GPIO_ENABLED

/* Private Typedef ---------------------------------------------------------------------*/
/** 
 *@brief GPIO对象私有成员结构体 
 */
typedef struct _S_MY_GPIO
{
	GPIO_TypeDef* 	GPIOx; 
	uint16_t 		GPIO_Pin;
}s_my_gpio;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_gpio_set(const c_my_gpio* p_obj, GPIO_PinState state);
static int my_gpio_toggle(const c_my_gpio* p_obj);
static int my_gpio_get(const c_my_gpio* p_obj, GPIO_PinState *state);
static int my_gpio_init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t Mode, uint32_t Pull, uint32_t Speed);

/* Private Variables -------------------------------------------------------------------*/
//参数定义: GPIO 单例模型，扩展到外部使用定义
const c_my_gpio_t my_gpio = {my_gpio_init};

/* Public Functions -------------------------------------------------------------------*/
/** 
 *@brief GPIO对象创建函数
 */
c_my_gpio my_gpio_create(GPIO_TypeDef* 	GPIOx, 
						uint16_t 		GPIO_Pin, 
						GPIO_INIT_TYPE 	type)
{
	GPIO_InitTypeDef GPIO_InitStruct;		/*GPIO初始化结构体*/	
	c_my_gpio new = {0};				/*新建一个空的GPIO对象*/	
	
	/*为GPIO新对象的私有成员申请内存*/
	new.this = MY_MALLOC(sizeof(s_my_gpio));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_my_gpio));

	/*初始化GPIO对象的私有成员*/
	((s_my_gpio*)new.this)->GPIOx 		= GPIOx;
	((s_my_gpio*)new.this)->GPIO_Pin 	= GPIO_Pin;
	
	/*初始化GPIO对象的公有接口*/
	new.set 	= my_gpio_set;
	new.toggle 	= my_gpio_toggle;
	new.get 	= my_gpio_get;
	
	/*初始化对应的GPIO*/
    GPIO_InitStruct.Pin   = GPIO_Pin 			 ;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP ;  //默认推挽输出
    GPIO_InitStruct.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
	if(type == GPIO_IN){
		GPIO_InitStruct.Mode  = GPIO_MODE_INPUT ;  //切换为输入
	}	
    HAL_GPIO_Init(GPIOx,&GPIO_InitStruct); 

	return new;
}

/* Private Functions -------------------------------------------------------------------*/
/** 
 *@brief 设置GPIO对象的状态
 */
static int my_gpio_set(const c_my_gpio* p_obj, GPIO_PinState state)
{
	const s_my_gpio* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置IO状态*/
	this = p_obj->this;
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, state);

	return	R_OK;
}

/** 
 *@brief 翻转GPIO对象的状态
 */
static int my_gpio_toggle(const c_my_gpio* p_obj)
{
	const s_my_gpio* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*翻转IO状态*/
	this = p_obj->this;
	HAL_GPIO_TogglePin(this->GPIOx, this->GPIO_Pin);
	
	return	R_OK;
}

/** 
 *@brief 获取GPIO对象的状态
 */
static int my_gpio_get(const c_my_gpio* p_obj, GPIO_PinState *state)
{
	const s_my_gpio* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*获取IO状态*/
	this = p_obj->this;
	*state = HAL_GPIO_ReadPin(this->GPIOx, this->GPIO_Pin);
	
	return	R_OK;
}

/** 
 *@brief 单例模型对外接口
 */
static int my_gpio_init(GPIO_TypeDef *GPIOx, 
				  uint16_t 		GPIO_Pin, 
				  uint32_t 		Mode, 
				  uint32_t 		Pull, 
				  uint32_t 		Speed)
{
	GPIO_InitTypeDef GPIO_InitStruct    = {0};
	GPIO_InitStruct.Pin                 = GPIO_Pin;
	GPIO_InitStruct.Mode                = Mode;
	GPIO_InitStruct.Pull                = Pull;
	GPIO_InitStruct.Speed               = Speed; 
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
	
	return	R_OK;
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
