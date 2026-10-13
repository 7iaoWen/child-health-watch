/********************************************************************************
* @file		:my_adc.c
* @brief	:对STM32 HAL 库中 ADC 的二次封装，提供 ADC 的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/02
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_ADC_ENABLED

#define MAX_ADC_CHANNEL	ADC_CHANNEL_9

/* Private Typedef  -------------------------------------------------------------------*/

/* Private Functions Declare ----------------------------------------------------------*/
static int my_adc_init(uint32_t xChn);
static int my_adc_get(uint32_t xChn, float* fVol);

/* Private Variables -------------------------------------------------------------------*/
//参数定义:	ADC单例模型，扩展到外部使用定义
const c_my_adc my_adc = {my_adc_init, my_adc_get};
//参数定义:	ADC句柄
static ADC_HandleTypeDef ADC_Handle;
//参数定义:	ADC初始化状态
static bool _adc_init_state = false;
static bool _adc_chn_init_state[10] = {false};
//参数定义:	ADC配置参数，通道对应引脚定义
static const struct {
	uint32_t		xChn;
	GPIO_TypeDef* 	GPIOx; 
	uint16_t 		GPIO_Pin;
} s_my_adc_cfg[] = {
	{ADC_CHANNEL_0, GPIOA, GPIO_PIN_0},
	{ADC_CHANNEL_1, GPIOA, GPIO_PIN_1},
	{ADC_CHANNEL_2, GPIOA, GPIO_PIN_2},
	{ADC_CHANNEL_3, GPIOA, GPIO_PIN_3},
	{ADC_CHANNEL_4, GPIOA, GPIO_PIN_4},
	{ADC_CHANNEL_5, GPIOA, GPIO_PIN_5},
	{ADC_CHANNEL_6, GPIOA, GPIO_PIN_6},
	{ADC_CHANNEL_7, GPIOA, GPIO_PIN_7},
	{ADC_CHANNEL_8, GPIOB, GPIO_PIN_0},
	{ADC_CHANNEL_9, GPIOB, GPIO_PIN_1},
};

/* Private Functions -------------------------------------------------------------------*/
/** 
 *@brief 设置初始化ADC通道的状态
 */
static int my_adc_init(uint32_t xChn)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	/*参数检查*/
	if(xChn > MAX_ADC_CHANNEL){
		return R_PARAM;
	}
	
    /*初始化相应IO*/
    GPIO_Initure.Pin  = s_my_adc_cfg[xChn].GPIO_Pin; 
    GPIO_Initure.Mode = GPIO_MODE_ANALOG;     //模拟
    GPIO_Initure.Pull = GPIO_NOPULL;          //不带上下拉
    HAL_GPIO_Init(s_my_adc_cfg[xChn].GPIOx,&GPIO_Initure);
    _adc_chn_init_state[xChn] = true;	/*对应的通道标记为初始化成功*/
	
	if(!_adc_init_state){
		/*使能 ADC1 外设时钟*/
		__HAL_RCC_ADC1_CLK_ENABLE();
		
		/*ADC 时钟初始化*/
		RCC_PeriphCLKInitTypeDef ADC_CLKInit;				
		ADC_CLKInit.AdcClockSelection 		= RCC_ADCPCLK2_DIV6;//分频因子6时钟为72M/6=12MHz
		ADC_CLKInit.PeriphClockSelection 	= RCC_PERIPHCLK_ADC;//ADC外设时钟
		HAL_RCCEx_PeriphCLKConfig(&ADC_CLKInit); 
		
		/*ADC 外设初始化*/
		ADC_Handle.Instance						= ADC1;
		ADC_Handle.Init.DataAlign				= ADC_DATAALIGN_RIGHT; 	//右对齐
		ADC_Handle.Init.ScanConvMode			= DISABLE;            	//非扫描模式
		ADC_Handle.Init.ContinuousConvMode		= DISABLE;          	//非连续转换（单次转换）
		ADC_Handle.Init.NbrOfConversion			= 1;              		//1个转换在规则序列中 也就是只转换规则序列1 
		ADC_Handle.Init.DiscontinuousConvMode	= DISABLE;            	//禁止不连续采样模式
		ADC_Handle.Init.NbrOfDiscConversion		= 0;                   	//不连续采样通道数为0
		ADC_Handle.Init.ExternalTrigConv		= ADC_SOFTWARE_START; 	//软件触发
		HAL_ADC_Init(&ADC_Handle);                                 		//初始化 
		HAL_ADCEx_Calibration_Start(&ADC_Handle);                  		//校准ADC	
		
		_adc_init_state = true;
	}

	return R_OK;
}

/** 
 *@brief 设置初始化ADC通道的状态
 */
static int my_adc_get(uint32_t xChn, float* fVol)
{
	ADC_ChannelConfTypeDef ADC1_ChanConf;
    uint32_t xadc_val;
    float fadc_val;

    /*检查参数*/
    if(NULL == fVol || xChn > MAX_ADC_CHANNEL){
        return R_PARAM;
    }

    /*检查初始化状态*/
    if(false == _adc_init_state || false == _adc_chn_init_state[xChn]){
        return R_ERROR;
    }

    /*配置ADC*/
    ADC1_ChanConf.Channel 		= xChn;                        		//通道
    ADC1_ChanConf.Rank			= 1;                      			//第1个序列，序列1
    ADC1_ChanConf.SamplingTime	= ADC_SAMPLETIME_239CYCLES_5;		//采样时间               
    HAL_ADC_ConfigChannel(&ADC_Handle, &ADC1_ChanConf);    			//通道配置
    
    HAL_ADC_Start(&ADC_Handle);             						//开启ADC
    
    HAL_ADC_PollForConversion(&ADC_Handle, 10);          			//轮询转换
 
    xadc_val 	= HAL_ADC_GetValue(&ADC_Handle);
    fadc_val   	= xadc_val * (3.3f / 4096.0f);
    *fVol     	= fadc_val;

    return R_OK;
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

