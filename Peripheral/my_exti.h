/********************************************************************************
* @file		:my_exti.h
* @brief	:my_exti.c头文件
* @Author	:Gwen9
* @Date		:2024/08/01
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_EXTI_H__
#define __MY_EXTI_H__

/* Public Macros  -------------------------------------------------------------------*/
#define MY_EXTI_FALLING  0 /*下降沿触发，默认为高电平*/
#define MY_EXTI_RISING   1 /*上升沿触发，默认为低电平*/

#define EXTI_TRIG        0  /*外部中断触发*/
#define EXTI_TRIG_LAST   2  /*外部中断 持续触发*/
#define EXTI_TRIG_CLEAR  1  /*外部中断 清除恢复*/

#define EXTI_TRIG_LAST_TIME   (1500)  //触发时间持续2000 ms以上， 触发中断

/* Public Typedef  -------------------------------------------------------------------*/


/*外部中断列表*/
typedef struct __EXTI_LIST
{
    const char*   name    ;    /*按键名称*/
    GPIO_TypeDef* GPIOx   ;    /*按键所在GPIO分组*/
    uint16_t      GPIO_Pin;    /*按键所在PIN脚*/
    uint8_t       Polarity;    /*按键极性*/
}exti_list_t;

typedef struct _MY_EXTI c_my_exti;
typedef struct _MY_EXTI
{
    /************************************************* 
    * Description: EXTI 外部中断线初始化函数
    * Input : <list> 需要配置为 EXTI 外部中断模式 的 GPIO 对象列表，如果对象引脚冲突则只会初始化第一个; 
    *         <list_len> GPIO对象列表长度;
    *         <callback> EXTI 中断触发后的回调函数;  
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  :  
            int my_exti_callback(const char* name, uint8_t state) {return R_OK;}
            exti_list_t exti_list[] = { //创建外部中断列表
              {"KEY1",  GPIOB, GPIO_PIN_10, MY_EXTI_FALLING },    //按键1， 下降沿触发，默认为高电平
              {"KEY2",  GPIOB ,GPIO_PIN_1 , MY_EXTI_RISING  },    //按键2， 上升沿触发，默认为低电平
              {"HW1",   GPIOB ,GPIO_PIN_1 , MY_EXTI_FALLING },    //红外1， 下降沿触发，默认为高电平
              {"FIRE1", GPIOB ,GPIO_PIN_1,  MY_EXTI_FALLING },    //火焰1， 下降沿触发，默认为高电平
            };
            my_exti.init(exti_list, sizeof(exti_list)/sizeof(exti_list_t), my_exti_callback);
    *************************************************/  
    int(*init)(exti_list_t* list, uint8_t list_len, int (*callback)(const char*, uint8_t));

    /************************************************* 
    * Description: EXTI 获取外部中断线触发次数
    * Input : <name> 外部中断线名称
              <count> 获取到的触发次数
              <clear> 获取后清零 true:清零， false:不清零
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  :  
        uint32_t trig_count;
        my_exti.get_trig_count("FIRE1", &trig_count, false);
    *************************************************/  
    int(*get_trig_count)(const char* name, uint32_t *trig_count, bool clear); 
}c_my_exti;
	
/** 
  * @brief  片上GPIO外设, 单例模型，扩展到外部使用
  */
extern const c_my_exti my_exti;

#endif 
