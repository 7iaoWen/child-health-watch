/********************************************************************************
* @file		:ds18b20.c
* @brief	:ds18b20 温度检测传感器驱动
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_DS18B20_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _S_DS18B20
{
	GPIO_TypeDef* 	GPIOx; 
	uint16_t 		GPIO_Pin;
    uint32_t      	m_old_tick ;  /*上一次采集的时间*/
    float         	m_temp_buf ;  /*温度缓存*/
    bool          	m_state    ;  /*传感器状态*/
}s_ds18b20;

/* Private Functions Declare ----------------------------------------------------------*/
static int ds18b20_get_temp(const c_ds18b20* p_obj, float *temp);
static int ds18b20_read_rom_id(const c_ds18b20* p_obj, uint8_t *rom_id);
static int ds18b20_get_temp_match_rom(const c_ds18b20* p_obj, float *temp, uint8_t *rom_id);

/* Private Variables -------------------------------------------------------------------*/
/* Public Functions -------------------------------------------------------------------*/
c_ds18b20 ds18b20_create(GPIO_TypeDef* GPIOx,uint32_t GPIO_Pin)
{
	c_ds18b20 new = {0};	//新建一个空的GPIO对象		
	
	/*为 DS18B20 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_ds18b20));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_ds18b20));

	/*初始化 DS18B20 对象的私有成员*/
	((s_ds18b20*)new.this)->GPIOx 		= GPIOx;
	((s_ds18b20*)new.this)->GPIO_Pin 	= GPIO_Pin;
	
	/*初始化 DS18B20 对象的公有接口*/
	new.get 				= ds18b20_get_temp;
	new.rom_id  			= ds18b20_read_rom_id;
	new.get_match_rom_id 	= ds18b20_get_temp_match_rom;
	
	/*初始化 DS18B20 对象用到的引脚*/	//DS18B20 需要拉高释放总线 是否需要配置为开漏输出？
	my_gpio.init(GPIOx, GPIO_Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	
	/*默认输出高电平*/
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);  
	
	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static int ds18b20_reset(const c_ds18b20* p_obj)
{
    s_ds18b20* this = NULL;
	uint16_t retry = 0;
	
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
    this = p_obj->this;

	/*DQ 设置为输出模式*/ 
	my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);	
	
	/*拉高先保证总线为空闲状态*/
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);
	DelayUs(20);
	
	/*主机至少产生480us的低电平复位信号*/
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_RESET);  
	DelayUs(750);
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);
	
	/*从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲*/
	DelayUs(15);
	
	/*DQ 设置为输入模式 检测从机给主机返回的存在脉冲*/
	my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	
	/*等待ds18b20拉低*/
    while(GPIO_PIN_SET == HAL_GPIO_ReadPin(this->GPIOx, this->GPIO_Pin) && retry < 200) {
        retry++;
        DelayUs(1);
    };

	/*如果超时了 返回错误*/
    if(retry>=200){
        goto error_handle;
    }else {
        retry = 0;
    }
	
	/*等待ds18b20 拉高*/
    while (GPIO_PIN_RESET == HAL_GPIO_ReadPin(this->GPIOx, this->GPIO_Pin) && retry < 240){
        retry++;
        DelayUs(1);
    };
	
    /*如果超时了 返回错误*/
    if(retry >= 240){
        goto error_handle;
    }
	
	return R_OK;
	
error_handle:
	/*输出模式*/
	my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);	
	/*拉高释放*/
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);
	return R_ERROR;
}

static int ds18b20_read_byte(const c_ds18b20* p_obj, uint8_t* data)
{
    s_ds18b20* this = NULL;
    uint8_t recv_data = 0;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this || NULL == data){
        return R_NULL;
    }
    this = p_obj->this;

	/*DS18B20 读取一个字节的数据*/
    for(uint8_t i = 0; i < 8; i++){
		/*设置为输出模式*/  //DS18B20 需要拉高释放总线 是否需要配置为开漏输出？
		my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
		
		/*读时间的起始：必须由主机产生 >1us <15us 的低电平信号 */
        HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_RESET);  
        DelayUs(10);
		
		/*设置成输入，释放总线，由外部上拉电阻将总线拉高 */
		my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
		
		/*单片机需要在读周期开始后的 15us 内读取引脚的电平大小，之后释放总线 45us 完成时序*/
		if(GPIO_PIN_SET == HAL_GPIO_ReadPin(this->GPIOx, this->GPIO_Pin)){
            recv_data |= 0x01 << i;
        }
        DelayUs(45);       
    }
    *data = recv_data;

    return R_OK;
}

static int ds18b20_write_byte(const c_ds18b20* p_obj, uint8_t data)
{
    s_ds18b20* this = NULL;
    uint8_t send_bit = 0;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
    this = p_obj->this;

	/*DQ 设置为输出模式*/  //DS18B20 需要拉高释放总线 是否需要配置为开漏输出？
	my_gpio.init(this->GPIOx, this->GPIO_Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	
	/*向 DS18B20 发送一个字节*/
	for(uint8_t i = 0; i < 8; i++){
        send_bit = data & 0x01;
        data >>= 1;		
		if(send_bit){	// Write 1
			HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_RESET);  //1us < 这个延时 < 15us
			DelayUs(2);                            
			HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);
			DelayUs(60);   		
		}else{			// Write 0
			HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_RESET); // 60us < Tx 0 < 120us
			DelayUs(70);                            
			HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);   //1us < Trec(恢复时间) < 无穷大
			DelayUs(2);  
		}
	}
	
    return R_OK;
}

static int ds18b20_get_temp(const c_ds18b20* p_obj, float *temp)
{
	s_ds18b20* this = NULL;
	int ret = 0;
	uint8_t tl = 0, th = 0;
	short s_temp = 0;
	float f_temp = 0.0;
	
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this || NULL == temp){
        return R_NULL;
    }
    this = p_obj->this;
	
    ret = ds18b20_reset(p_obj);				//启动通讯
	if(R_OK != ret)	goto error_handle;
    
    ret = ds18b20_write_byte(p_obj, 0xcc);	//跳过ROM
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_write_byte(p_obj, 0x44);	//开始转换
	if(R_OK != ret)	goto error_handle;

    /*等待转换完成*/

    ret = ds18b20_reset(p_obj);				//启动通讯
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_write_byte(p_obj, 0xcc);	//跳过ROM
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_write_byte(p_obj, 0xbe);	//发送读命令
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_read_byte(p_obj, &tl);		//读取温度低字节 LSB
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_read_byte(p_obj, &th);		//读取温度高字节 MSB
	if(R_OK != ret)	goto error_handle;

	/*计算结果*/
	s_temp = th << 8;
	s_temp = s_temp | tl;
	
	if(s_temp < 0) 	f_temp = (~s_temp+1) * 0.0625;		/* 负温度 */
	else			f_temp = s_temp * 0.0625;
		
	*temp = f_temp;	
	return R_OK;
	
error_handle:
    return R_ERROR;
	//以前的计算方法
//    /*判断温度正负号*/
//    if(th>7){
//        th=~th;
//        tl=~tl; 
//        temp_polarity = -1.0f;      //温度为负  
//    }else{
//        temp_polarity =  1.0f;      //温度为正    
//    }

//    /*合并高低位*/
//    temp_16 = th;                     //获得高八位
//    temp_16 <<= 8;    
//    temp_16 += tl;                    //获得底八位

//    /*转换成浮点*/
//    temp_float = (float)temp_16 * 0.625f / 10.0f;
//    *temp = temp_float;
}

static int ds18b20_read_rom_id(const c_ds18b20* p_obj, uint8_t *rom_id)
{
	s_ds18b20* this = NULL;
	int ret = 0;
	
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this || NULL == rom_id){
        return R_NULL;
    }
    this = p_obj->this;
	
	ret = ds18b20_write_byte(p_obj, 0x33);	//读取序列号
	if(R_OK != ret)	goto error_handle;
	
	/*读取8个字节的 ROM ID*/
	for (uint8_t i = 0; i < 8; i++){
		ds18b20_read_byte(p_obj, rom_id+i);
	}
	return R_OK;
	
error_handle:
    return R_ERROR;
}

static int ds18b20_get_temp_match_rom(const c_ds18b20* p_obj, float *temp, uint8_t *rom_id)
{
	s_ds18b20* this = NULL;
	int ret = 0;
	short s_temp = 0;
	float f_temp = 0.0;
	uint8_t tl = 0, th = 0;
	
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
    this = p_obj->this;
	
	//=======================
	ret = ds18b20_reset(p_obj);				//启动通讯
	if(R_OK != ret)	goto error_handle;
    
    ret = ds18b20_write_byte(p_obj, 0x55);	//匹配ROM
	if(R_OK != ret)	goto error_handle;

	/*写入目标ID*/
	for(uint8_t i = 0; i < 8; i++){		
		ret = ds18b20_write_byte(p_obj, rom_id[i]);
	}
		
	ret = ds18b20_write_byte(p_obj, 0x44);	//开始转换
	if(R_OK != ret)	goto error_handle;

	//=======================
	ret = ds18b20_reset(p_obj);				//启动通讯
	if(R_OK != ret)	goto error_handle;
    
    ret = ds18b20_write_byte(p_obj, 0x55);	//匹配ROM
	if(R_OK != ret)	goto error_handle;

	/*写入目标ID*/
	for(uint8_t i = 0; i < 8; i++){		
		ret = ds18b20_write_byte(p_obj, rom_id[i]);
	}
	
	ret = ds18b20_write_byte(p_obj, 0xbe);	//发送读命令
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_read_byte(p_obj, &tl);		//读取温度低字节 LSB
	if(R_OK != ret)	goto error_handle;

    ret = ds18b20_read_byte(p_obj, &th);		//读取温度高字节 MSB
	if(R_OK != ret)	goto error_handle;

	/*计算结果*/
	s_temp = th << 8;
	s_temp = s_temp | tl;
	
	if(s_temp < 0) 	f_temp = (~s_temp+1) * 0.0625;		/* 负温度 */
	else			f_temp = s_temp * 0.0625;
		
	*temp = f_temp;	
	
	return R_OK;
	
error_handle:
    return R_ERROR;
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/********************************************************************************
参考博客：https://blog.csdn.net/weixin_45829131/article/details/108258260

1. DS18B20采用单总线协议，即与单片机借口仅需占用一个I/O端口，无需任何外部元件，仅需要一个上拉电阻，
即可将外部环境温度以数字码的方式串行输出，从而大大简化了传感器与微处理器的接口，
且DS18B20提供了9—12位的可编程分辨率  可分辨温度分别为0.5°C,0.25°C,0.125°C,0.0625°C,
其转化速率随着温度分辨率的增加而减少。并且DS18B20的测温范围在 -55°C~+125°C 满足了大部分基本的项目需求。


2. DS18B20的基本读写初始化时序操作
--初始化操作
 -单片机先将DQ引脚拉至低电平480—960us发送一个低电平脉冲（复位）;
 -之后释放总线由上拉电阻将引脚上拉至高电平持续等待15—60us;
 -之后将单片机引脚转为输入模式检查DS18B20是否做出应答;
 -若DS18B20存在，则会将总线拉低60—240us将DQ引脚拉低做出应答;

--写数据操作
 -DS18B20的写周期最少为60us最长不能超过120us，
 -DS18B20写0时序与写1时序类似，
 -写0时序单片机将DQ引脚拉低之后DS18B20会在引脚被拉低的第15—60us内进行采样读取到写0的操作，
 -写1时序单片机将DQ引脚拉低1us的时间之后将总线释放由上拉电阻将总线拉直高电平，DS18B20则会在第15—60us内读取到写1操作

--读数据操作
 -由时序图可知读操作最短时间为60us最长不超过120us，
 -读操作开始时需要将DQ引脚拉低至少1us后释放总线，
 -在总线释放期间，若DS18B20发送0，则把总线拉低至少到15us，之后释放总线，若发送1，则不拉低总线，
 -单片机需要在读周期开始后的15us内读取引脚的电平大小，之后释放总线45us完成时序

3. DS18B20的ROM指令和存储器指令
--在完成对DS18B20的初始化后要对DS18B20进行操作，而DS18B20的指令可根据作用对象可分为ROM指令和存储器指令，
前者可通过ROM指令选择所需操作的DS18B20，后者是对所选的DS18B20进行温度读取，转换等具体操作。

--多个DS18B20： 当单片机上挂载了多个DS18B20，需要对某个进行单独操作时需要先将主机逐个与DS18B20连接，
独处其序列号，然后再将所有的DS18B20挂载到总线上，之后由单片机发出匹配ROM指命令（55H）。
紧接着单片机提供64位序列（包括该DS18B20的48位序列号）之后的操作就是对该DS18B20进行操作

--单个DS18B20：如果单片机上仅挂载了一个DS18B20仅需要发送跳过ROM指令（CCH）命令即可，
不需要进行读取ROM编码以及匹配ROM编码。

*******************************************************************************/


