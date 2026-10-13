/********************************************************************************
* @file		:my_usart.c
* @brief	:对STM32 HAL 库中 UART 串口 的二次封装，提供 UART 操作的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_UART_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _S_MY_UART
{
    my_uart_id_t       	uart_id		;  /*串口id*/
    bool              	init_state 	;  /*初始化状态*/
    uart_mode_t        	work_mode   ;  /*串口工作模式*/
	uint32_t			rx_cnt		;  /*串口接收计数*/
	uint8_t* 			rx_buf		;  /*接收数据缓存*/
	uint8_t*			p_head		;  /*写缓存指针*/
	uint8_t*			p_end		;  /*接收数据缓存结束地址*/
	uint32_t			rx_buf_size	;  /*接收缓存大小*/
    UART_HandleTypeDef 	huart   	;  /*串口句柄*/
    DMA_HandleTypeDef  	htxdma 		;  /*发送dma 句柄*/
    DMA_HandleTypeDef  	hrxdma  	;  /*接收dma 句柄*/
    void*              	callback_param;  /*回调函数参数*/
    void                (*callback)(void* param, uint8_t* data, uint16_t data_len);  /*接收回调函数*/
}s_my_uart;

typedef struct __UART_CFG
{  
    USART_TypeDef*       uart_addr    ;   /*串口地址*/
    DMA_Channel_TypeDef* txdma_channel;   /*发送DMA通道*/
    DMA_Channel_TypeDef* rxdma_channel;   /*接收DMA通道*/
	dma_chn_num_t		 txdma_num;
	dma_chn_num_t		 rxdma_num;
}uart_cfg;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_uart_init(my_uart_id_t uart_id, uint32_t baud_rate, uart_mode_t mode);
static int my_uart_send(my_uart_id_t uart_id, uint8_t *data, uint32_t len);
static int my_uart_recive(my_uart_id_t uart_id, uint8_t *data, uint32_t *len);
static void my_uart_period_func(void *param);
static int my_uart_register_callback(my_uart_id_t uart_id, void(*callback)(void* , uint8_t* , uint16_t), void *param);
static void my_uart_txdma_interrupt(dma_chn_num_t dma_chn_num, void* param);
static void my_uart_rxdma_interrupt(dma_chn_num_t dma_chn_num, void* param);
/* Private Variables -------------------------------------------------------------------*/
//参数定义: UART 串口单例模型
const c_my_uart my_uart = {my_uart_init, my_uart_send, my_uart_recive, my_uart_register_callback};
//参数定义: 串口配置 
const static uart_cfg sys_cfg[UART_MAX] = {
	{USART1, DMA1_Channel4, DMA1_Channel5, DMAChn4, DMAChn5},
	{USART2, DMA1_Channel7, DMA1_Channel6, DMAChn7, DMAChn6},
	{USART3, DMA1_Channel2, DMA1_Channel3, DMAChn2, DMAChn3},
};
//参数定义: 串口 对象列表 	
static s_my_uart g_my_uart[UART_MAX];
//参数定义: 串口 接收数据缓存 	
static uint8_t uart1_rx_buf[UART1_RX_BUF_SIZE];
static uint8_t uart2_rx_buf[UART2_RX_BUF_SIZE];
static uint8_t uart3_rx_buf[UART3_RX_BUF_SIZE];
static uint8_t uart_temp_buf[UART_RX_BUF_MAX_SIZE];
//参数定义: 串口 接收数据缓存大小数组
static const uint32_t rx_buf_size[UART_MAX] = {UART1_RX_BUF_SIZE, UART2_RX_BUF_SIZE, UART3_RX_BUF_SIZE};

//参数定义: 串口 周期任务函数注册标志位（只需要注册一次，所以在第一个串口初始化时置位即可）
static bool my_uart_period_task_register_flag = false;
/* Private Functions -------------------------------------------------------------------*/
/** 
 *@brief 串口初始化函数
 */
static int my_uart_init(my_uart_id_t uart_id, uint32_t baud_rate, uart_mode_t mode)
{
	HAL_StatusTypeDef ret  = HAL_ERROR ;
	
	/*检查参数*/
    if(uart_id >= UART_MAX){
        return R_PARAM;
    }
	
	/*检查初始化状态*/
    if(g_my_uart[uart_id].init_state){
        return R_OK;
    }
	
	/*私有成员初始化化*/
	g_my_uart[uart_id].uart_id   	= uart_id;
    g_my_uart[uart_id].work_mode  	= mode;
	g_my_uart[uart_id].rx_cnt		= 0;
	g_my_uart[uart_id].rx_buf_size	= rx_buf_size[uart_id];
	if(uart_id == MY_UART1){
		g_my_uart[uart_id].rx_buf 	= uart1_rx_buf;
		g_my_uart[uart_id].p_end	= uart1_rx_buf;
	}else if(uart_id == MY_UART2){
		g_my_uart[uart_id].rx_buf 	= uart2_rx_buf;
		g_my_uart[uart_id].p_end	= uart2_rx_buf;
	}else if(uart_id == MY_UART3){
		g_my_uart[uart_id].rx_buf 	= uart3_rx_buf;
		g_my_uart[uart_id].p_end	= uart3_rx_buf;
	}	
	g_my_uart[uart_id].p_head		= g_my_uart[uart_id].rx_buf;
	
	/*初始化串口*/
    g_my_uart[uart_id].huart.Instance        = sys_cfg[uart_id].uart_addr  		;
    g_my_uart[uart_id].huart.Init.BaudRate   = baud_rate                        ; //波特率
    g_my_uart[uart_id].huart.Init.WordLength = UART_WORDLENGTH_8B               ; //字长为8位数据格式
    g_my_uart[uart_id].huart.Init.StopBits   = UART_STOPBITS_1                  ; //一个停止位
    g_my_uart[uart_id].huart.Init.Parity     = UART_PARITY_NONE                 ; //无奇偶校验位
    g_my_uart[uart_id].huart.Init.HwFlowCtl  = UART_HWCONTROL_NONE              ; //无硬件流控
    g_my_uart[uart_id].huart.Init.Mode       = UART_MODE_TX_RX                  ; //收发模式
	
    ret = HAL_UART_Init(&g_my_uart[uart_id].huart)             					; //HAL_UART_Init()会使能UART1
    if(HAL_OK != ret){
		return R_ERROR;
	}	
	
	/*不同模式下的初始化*/
    if(mode == UART_MODE_ISR){
        __HAL_UART_ENABLE_IT(&g_my_uart[uart_id].huart, UART_IT_RXNE); 
    }else{
        /*初始化 发送DMA*/
        __HAL_LINKDMA(&g_my_uart[uart_id].huart, hdmatx, g_my_uart[uart_id].htxdma);
        g_my_uart[uart_id].htxdma.Instance                 = sys_cfg[uart_id].txdma_channel ;   //通道选择
        g_my_uart[uart_id].htxdma.Init.Direction           = DMA_MEMORY_TO_PERIPH ;   //存储器到外设
        g_my_uart[uart_id].htxdma.Init.PeriphInc           = DMA_PINC_DISABLE     ;   //外设非增量模式
        g_my_uart[uart_id].htxdma.Init.MemInc              = DMA_MINC_ENABLE      ;   //存储器增量模式
        g_my_uart[uart_id].htxdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE  ;   //外设数据长度:8位
        g_my_uart[uart_id].htxdma.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE  ;   //存储器数据长度:8位
        g_my_uart[uart_id].htxdma.Init.Mode                = DMA_NORMAL           ;   //外设普通模式
        g_my_uart[uart_id].htxdma.Init.Priority            = DMA_PRIORITY_MEDIUM  ;   //中等优先级  
        HAL_DMA_Init  (&g_my_uart[uart_id].htxdma);  /*初始化DMA*/
		
        /*初始化 接收DMA*/
        __HAL_LINKDMA(&g_my_uart[uart_id].huart, hdmarx, g_my_uart[uart_id].hrxdma);
        g_my_uart[uart_id].hrxdma.Instance                 = sys_cfg[uart_id].rxdma_channel       ;   //通道选择
        g_my_uart[uart_id].hrxdma.Init.Direction           = DMA_PERIPH_TO_MEMORY ;   //外设到存储器
        g_my_uart[uart_id].hrxdma.Init.PeriphInc           = DMA_PINC_DISABLE     ;   //外设非增量模式
        g_my_uart[uart_id].hrxdma.Init.MemInc              = DMA_MINC_ENABLE      ;   //存储器增量模式
        g_my_uart[uart_id].hrxdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE  ;   //外设数据长度:8位
        g_my_uart[uart_id].hrxdma.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE  ;   //存储器数据长度:8位
        g_my_uart[uart_id].hrxdma.Init.Mode                = DMA_CIRCULAR         ;   //外设循环模式
        g_my_uart[uart_id].hrxdma.Init.Priority            = DMA_PRIORITY_HIGH  ;     //中等优先级  
        HAL_DMA_Init  (&g_my_uart[uart_id].hrxdma);  		/*初始化DMA*/
		
		/*注册发送/接收DMA回调函数*/
		it_register_dma(sys_cfg[uart_id].txdma_num, my_uart_txdma_interrupt, &g_my_uart[uart_id]);
		it_register_dma(sys_cfg[uart_id].rxdma_num, my_uart_rxdma_interrupt, &g_my_uart[uart_id]);
		
		/*启动DMA接收 注:接收DMA就在此处启动一次，后续不会关闭，将一直开启传输*/
		HAL_UART_Receive_DMA(&g_my_uart[uart_id].huart, g_my_uart[uart_id].rx_buf, rx_buf_size[uart_id]);
    }

	/*注册100ms周期任务函数，注册成功后需要在 while 中调用 my_systick.period_task_running(); 才会执行注册的函数*/
	if(!my_uart_period_task_register_flag){
		ret = my_systick.period_task_register(MY_SYSTICK_PERIOD_100MS, my_uart_period_func, NULL);
		if(HAL_OK != ret){
			return R_ERROR;
		}
		my_uart_period_task_register_flag = true;		
	}

	g_my_uart[uart_id].init_state = true;

	return R_OK;
}
	
/** 
 *@brief 串口发送函数
 */
static int my_uart_send(my_uart_id_t uart_id, uint8_t *data, uint32_t len)
{
	HAL_StatusTypeDef ret  = R_OK;
	
	/*检查参数*/
    if(uart_id >= UART_MAX){
        return R_PARAM;
    }
	
	/*检查初始化状态*/
    if(!g_my_uart[uart_id].init_state){
        return R_ERROR;
    }

	if(g_my_uart[uart_id].work_mode == UART_MODE_DMA){
		while(HAL_UART_GetState(&g_my_uart[uart_id].huart) != HAL_UART_STATE_READY
			&& HAL_UART_GetState(&g_my_uart[uart_id].huart) != HAL_UART_STATE_BUSY_RX){
			;
		}
		ret = HAL_UART_Transmit_DMA(&g_my_uart[uart_id].huart, data, len);
	}else{
		/*寄存器操作直接发送数据*/
		for(uint32_t cnt = 0; cnt < len; cnt++)
		{
			while((g_my_uart[uart_id].huart.Instance->SR & 0x40) == 0);	
			g_my_uart[uart_id].huart.Instance->DR = *(data + cnt);
			while((g_my_uart[uart_id].huart.Instance->SR & 0x40) == 0);	
		}
	}
	return ret;
}

/** 
 *@brief 串口接收函数
 */
static int my_uart_recive(my_uart_id_t uart_id, uint8_t *data, uint32_t *len)
{
	uint32_t recv_count;
	
	/*检查参数*/
    if(uart_id >= UART_MAX){
        return R_PARAM;
    }

	/*检查初始化状态*/
    if(!g_my_uart[uart_id].init_state){
        return R_ERROR;
    }
	
	*len = 0;
	/*数据接收处理*/
	if(g_my_uart[uart_id].work_mode != UART_MODE_ISR){	
		//DMA模式
		recv_count = __HAL_DMA_GET_COUNTER(&g_my_uart[uart_id].hrxdma); 		//获取DMA剩余计数
		recv_count = rx_buf_size[uart_id] - recv_count;							//计算已经收到的数据长度
		g_my_uart[uart_id].p_end = g_my_uart[uart_id].rx_buf + recv_count;		//计算数据结尾
	}else{
		//ISR中断模式
		recv_count = g_my_uart[uart_id].rx_cnt;									//已经收到的数据长度
		g_my_uart[uart_id].p_end = g_my_uart[uart_id].rx_buf + recv_count;		//计算数据结尾
	}

    
	if(g_my_uart[uart_id].p_head < g_my_uart[uart_id].p_end){
		 /*如果 尾在头后面 则数据没有溢出,直接拷贝数据到临时缓存种*/ 
		recv_count = g_my_uart[uart_id].p_end - g_my_uart[uart_id].p_head;
		if(recv_count > rx_buf_size[uart_id]) recv_count = rx_buf_size[uart_id];	//限幅
		memset(uart_temp_buf, 0, sizeof(uart_temp_buf));
		memcpy(uart_temp_buf, g_my_uart[uart_id].p_head, recv_count);
	}else if(g_my_uart[uart_id].p_head > g_my_uart[uart_id].p_end){
		/*如果 尾在头前面 则数据已经溢出，需要重新拼接数据*/
		recv_count = g_my_uart[uart_id].rx_buf_size - (g_my_uart[uart_id].p_head - g_my_uart[uart_id].p_end);
		if(recv_count > rx_buf_size[uart_id]) recv_count = rx_buf_size[uart_id];	//限幅
		memset(uart_temp_buf, 0, sizeof(uart_temp_buf));
		uint32_t temp = g_my_uart[uart_id].rx_buf_size - (g_my_uart[uart_id].p_head - g_my_uart[uart_id].rx_buf);//计算被覆盖的数据长度
		memcpy(uart_temp_buf, g_my_uart[uart_id].p_head, temp);														//拷贝前段
		memcpy(uart_temp_buf + temp, g_my_uart[uart_id].rx_buf, g_my_uart[uart_id].p_end - g_my_uart[uart_id].rx_buf);	//拷贝后段
	}else{
		return R_OK;
	}

	/*更新头部指针位置*/
	g_my_uart[uart_id].p_head = g_my_uart[uart_id].p_end;
	
	/*调用回调函数*/
	if(g_my_uart[uart_id].callback != NULL){//不需要接收数据，这里先判断串口回调函数是否为空，不为空则调用回调
		g_my_uart[uart_id].callback(g_my_uart[uart_id].callback_param, uart_temp_buf, recv_count);
	}
	
	if(data != NULL && len != NULL){
		*len = recv_count;
		memcpy(data, uart_temp_buf, recv_count);
	}

    return R_OK;
}

/** 
 *@brief 周期轮询函数三个串口，如果收到数据则调用回调函数
 */
static void my_uart_period_func(void *param)
{
	for(my_uart_id_t i = MY_UART1; i < UART_MAX; i++){
		/*检查初始化状态*/
		if(!g_my_uart[i].init_state)	continue;
		my_uart_recive(i, NULL, NULL);				//不接收数据，只再收到执行串口接收回调函数
	}
}

/** 
 *@brief 串口接收函数
 */
static int my_uart_register_callback(my_uart_id_t uart_id, void(*callback)(void* , uint8_t* , uint16_t), void *param)
{
	/*检查参数*/
	if(callback == NULL){
		return R_NULL;
	}
    if(uart_id >= UART_MAX){
        return R_PARAM;
    }
	/*检查初始化状态*/
    if(!g_my_uart[uart_id].init_state){
        return R_ERROR;
    }
	/*注册串口接收回调函数*/
	g_my_uart[uart_id].callback_param = param;
	g_my_uart[uart_id].callback		  = callback;
	
    return R_OK;
}


/** 
 *@brief 底层MSP初始化：IO/时钟/中断控制器
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if(huart->Instance==USART1)					//如果是串口1，进行串口1 MSP初始化
    {
        __HAL_RCC_USART1_CLK_ENABLE();          //使能USART1时钟
		
		my_gpio.init(GPIOA, GPIO_PIN_9,  GPIO_MODE_AF_PP, 	 GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
		my_gpio.init(GPIOA, GPIO_PIN_10, GPIO_MODE_AF_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);

		if(g_my_uart[MY_UART1].work_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART1_IRQn);   
		}
    }
    else if(huart->Instance==USART2)			//如果是串口2，进行串口2 MSP初始化
    {
        __HAL_RCC_USART2_CLK_ENABLE();          //使能USART1时钟
		
		my_gpio.init(GPIOA, GPIO_PIN_2, GPIO_MODE_AF_PP, 	GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
		my_gpio.init(GPIOA, GPIO_PIN_3, GPIO_MODE_AF_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);

		if(g_my_uart[MY_UART2].work_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART2_IRQn);   
		}	
    }
    else if(huart->Instance==USART3)			//如果是串口3，进行串口3 MSP初始化
    {
        __HAL_RCC_USART3_CLK_ENABLE();          //使能USART1时钟
		
		my_gpio.init(GPIOB, GPIO_PIN_10, GPIO_MODE_AF_PP, 	 GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
		my_gpio.init(GPIOB, GPIO_PIN_11, GPIO_MODE_AF_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);

		if(g_my_uart[MY_UART3].work_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART3_IRQn);   
		} 		
    }
}

/** 
 *@brief 串口DMA中断收发处理
 */
static void my_uart_txdma_interrupt(dma_chn_num_t dma_chn_num, void* param)
{
    s_my_uart* this = NULL;
	
    /*检查参数*/
    if(NULL == param){
        return;
    }
	this = param;
	
    /*如果触发了发送完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&this->htxdma, __HAL_DMA_GET_TC_FLAG_INDEX(&this->htxdma)) != RESET)
    {       
		/*停止发送DMA请求 并关闭发送DMA*/
		CLEAR_BIT(this->huart.Instance->CR3, USART_CR3_DMAT);
		HAL_DMA_Abort(&this->htxdma);
	
		/*因为上文中 发送停止了，所以 清楚串口状态中的 发送忙状态*/
		if(HAL_UART_STATE_BUSY_TX == this->huart.gState){
			this->huart.gState = HAL_UART_STATE_READY;
		}else if(HAL_UART_STATE_BUSY_TX_RX == this->huart.gState){
			this->huart.gState = HAL_UART_STATE_BUSY_RX;
		}
		
    }
    HAL_DMA_IRQHandler(&this->htxdma);
}

static void my_uart_rxdma_interrupt(dma_chn_num_t dma_chn_num,void* param)
{
    s_my_uart* this = NULL;
    /*检查参数*/
    if(NULL == param){
        return;
    }
	this = param;
	
    /*如果触发了接收完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&this->hrxdma, __HAL_DMA_GET_TC_FLAG_INDEX(&this->hrxdma)) != RESET)
    {
        /*发送任务通知 立即查看DMA接收内容*/
    }
    
    HAL_DMA_IRQHandler(&this->hrxdma);
}


/** 
 *@brief 串口中断接收处理函数
 */
void USART1_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART1].huart, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART1].huart.Instance->DR);	//读取数据
        if(++g_my_uart[MY_UART1].rx_cnt >= g_my_uart[MY_UART1].rx_buf_size) g_my_uart[MY_UART1].rx_cnt = 0;
        g_my_uart[MY_UART1].rx_buf[g_my_uart[MY_UART1].rx_cnt] = nRec;				//转存数据
    }
}   

void USART2_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART2].huart, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART2].huart.Instance->DR);	//读取数据
        if(++g_my_uart[MY_UART2].rx_cnt >= g_my_uart[MY_UART2].rx_buf_size) g_my_uart[MY_UART2].rx_cnt = 0;
        g_my_uart[MY_UART2].rx_buf[g_my_uart[MY_UART2].rx_cnt] = nRec;				//转存数据
    }
}

void USART3_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART3].huart, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART3].huart.Instance->DR);	//读取数据
        if(++g_my_uart[MY_UART3].rx_cnt >= g_my_uart[MY_UART3].rx_buf_size) g_my_uart[MY_UART3].rx_cnt = 0;
        g_my_uart[MY_UART3].rx_buf[g_my_uart[MY_UART3].rx_cnt] = nRec;				//转存数据
    }
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

