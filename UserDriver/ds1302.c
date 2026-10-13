/********************************************************************************
* @file		:hx711.c
* @brief	:hx711 A/D转换模块驱动程序，适用与同系列的 HX710 等
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_HX711_ENABLED

/* Private Macros  -------------------------------------------------------------------*/
#define LOW				GPIO_PIN_RESET
#define HIGH			GPIO_PIN_SET

//通信IO 操作宏
#define DAT_SET_MODE_IN()   my_gpio.init(this->dat_gpio, this->dat_pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH)

#define DAT_SET_MODE_OUT()  my_gpio.init(this->dat_gpio, this->dat_pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH)

#define RST_CTRL(n)  		HAL_GPIO_WritePin(this->rst_gpio, this->rst_pin, n)

#define CLK_CTRL(n)  		HAL_GPIO_WritePin(this->clk_gpio, this->clk_pin, n)

#define DAT_CTRL(n)  		HAL_GPIO_WritePin(this->dat_gpio, this->dat_pin, n)

#define DAT_READ()   		HAL_GPIO_ReadPin(this->dat_gpio, this->dat_pin)
	
//转码
#define BCD_TO_DEC(X) 		((X>>4)*10 + (X&0X0F))
	
#define DEC_TO_BCD(X) 		(((X/10)<<4) | (X%10))

#define DS1302_CYCLE        5

#define DS1302_RAM_MAX_ADDR 0x01
/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _S_DS1302
{
	GPIO_TypeDef* 	clk_gpio; 
	uint16_t 		clk_pin;
	GPIO_TypeDef* 	dat_gpio; 
	uint16_t 		dat_pin;
	GPIO_TypeDef* 	rst_gpio; 
	uint16_t 		rst_pin;
}s_ds1302;

/* Private Functions Declare ----------------------------------------------------------*/
static int ds1302_get_ram(const c_ds1302* p_obj, uint8_t addr, uint8_t *data);
static int ds1302_set_ram(const c_ds1302* p_obj, uint8_t addr, uint8_t data);
static int ds1302_get_time(const c_ds1302* p_obj, sys_time_t *time);
static int ds1302_set_time(const c_ds1302* p_obj, sys_time_t *time);

/* Private Variables ------------------------------------------------------------------*/

/* Public Functions -------------------------------------------------------------------*/
c_ds1302 ds1302_create(GPIO_TypeDef* clk_gpio, uint16_t clk_pin, 
					   GPIO_TypeDef* dat_gpio, uint16_t dat_pin,
					   GPIO_TypeDef* rst_gpio, uint16_t rst_pin)
{
	c_ds1302 new = {0};	//新建一个空的GPIO对象		
	
	/*为 DS1302 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_ds1302));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_ds1302));

	/*初始化 DS1302 对象的私有成员*/
	((s_ds1302*)new.this)->clk_gpio 	= clk_gpio;
	((s_ds1302*)new.this)->clk_pin 		= clk_pin;
	((s_ds1302*)new.this)->dat_gpio 	= dat_gpio;
	((s_ds1302*)new.this)->dat_pin 		= dat_pin;
	((s_ds1302*)new.this)->rst_gpio 	= rst_gpio;
	((s_ds1302*)new.this)->rst_pin 		= rst_pin;
	
	/*初始化 DS1302 对象的公有接口*/
	new.get_ram 		= ds1302_get_ram;
	new.set_ram 		= ds1302_set_ram;
	new.get_time		= ds1302_get_time;
	new.set_time		= ds1302_set_time;
	
	/*初始化 DS1302 对象用到的引脚*/
	my_gpio.init(clk_gpio, clk_pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
	my_gpio.init(dat_gpio, dat_pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);	//上拉
	my_gpio.init(rst_gpio, rst_pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
	
	/*
	-DS1302 RAM中的值判断 DS1302 是否已经设置过时间;
	-如果之前有设置过，则 ram 中存在值 DS1302_TIME_VAILID，此时不做操作
	-如果之前没有设置过时间，则重设时间为默认时间
	*/
	uint8_t read_data = 0;
    sys_time_t time;
	new.get_ram(&new, DS1302_RAM_MAX_ADDR, &read_data);
    if(read_data != DS1302_TIME_VAILID){
        time.year 		= DS1302_DEF_YEAR;
        time.mon 		= DS1302_DEF_MON;
        time.date 		= DS1302_DEF_DATE;
        time.hour 		= DS1302_DEF_HOUR;
        time.min 		= DS1302_DEF_MIN;
        time.sec 		= DS1302_DEF_SEC;
		time.weekday	= DS1302_DEF_WEEK;
        new.set_time(&new, &time);
//        new.get_time(&new, &time);
		new.set_ram(&new, DS1302_RAM_MAX_ADDR, DS1302_TIME_VAILID);
    }else{
//        new.get_time(&new, &time);
    }

	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static uint8_t ds1302_read_register(const c_ds1302* p_obj, uint8_t addr)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
    this = p_obj->this;	
	
	uint8_t ret = 0;
	
    CLK_CTRL(LOW);          	//拉低时钟线
    DAT_SET_MODE_OUT();			//数据线配置为输出
    RST_CTRL(HIGH);          	//拉高复位

    for(uint8_t i = 0; i < 8; i++){
        (addr&0x01) == 0 ? DAT_CTRL(LOW) : DAT_CTRL(HIGH); 	//发送一位数据
        addr >>= 1;
        DelayUs(DS1302_CYCLE);             				//等待数据读取完成
        CLK_CTRL(HIGH);                  					//拉高时钟线，通知GM读取数据
        DelayUs(DS1302_CYCLE);            					//等待数据读取完成
        if(i == 7)  DAT_SET_MODE_IN();    					//在clk拉低之前执行，防止rtc数据传输过来时，引脚仍为输出
        CLK_CTRL(LOW);                 	 					//拉低时钟线
    }

    for(uint8_t i = 0; i < 8; i++){
        ret = ret >> 1; 						//读数据变量
        if(DAT_READ() == 0) ret &= 0x7f;
        else                ret |= 0x80;
        CLK_CTRL(HIGH);                  		//拉高时钟线，通知GM读取数据
        DelayUs(DS1302_CYCLE);             	//等待数据读取完成
        CLK_CTRL(LOW);                  		//拉高时钟线，通知GM读取数据
//        mDelayuS(CYCLE);             			//等待数据读取完成
    }

    RST_CTRL(LOW);          //拉复位
	
    return ret;
}

static void ds1302_write_register(const c_ds1302* p_obj, uint8_t addr, uint8_t data)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return ;
    }
    this = p_obj->this;	
	
    CLK_CTRL(LOW);          	//拉低时钟线
    DAT_SET_MODE_OUT();     	//IO配置为输出模式
    RST_CTRL(HIGH);          	//拉高复位

	/*发送地址*/
	for(uint8_t i = 0; i < 8; i++){
        (addr&0x01) == 0 ? DAT_CTRL(LOW) : DAT_CTRL(HIGH); 	//发送一位数据
        DelayUs(DS1302_CYCLE);      						//等待数据读取完成
        CLK_CTRL(HIGH);            							//拉高时钟线，通知GM读取数据
        DelayUs(DS1302_CYCLE);      						//等待数据读取完成
        CLK_CTRL(LOW);           				 			//拉低时钟线，准备发送下一个数据
        addr >>= 1;
    }
	/*发送数据*/
	for(uint8_t i = 0; i < 8; i++){
        (data&0x01) == 0 ? DAT_CTRL(LOW) : DAT_CTRL(HIGH); 	//发送一位数据
        DelayUs(DS1302_CYCLE);      						//等待数据读取完成
        CLK_CTRL(HIGH);            							//拉高时钟线，通知GM读取数据
        DelayUs(DS1302_CYCLE);     						//等待数据读取完成
        CLK_CTRL(LOW);            							//拉低时钟线，准备发送下一个数据
        data >>= 1;
    }

    DAT_CTRL(LOW);		  		//拉低数据
    DAT_SET_MODE_IN();	 		//数据线配置为输入模式
    RST_CTRL(LOW);          	//拉低复位
}
static int ds1302_get_ram(const c_ds1302* p_obj, uint8_t addr, uint8_t *data)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	if(addr > DS1302_RAM_MAX_ADDR){
        return R_PARAM;
    }
	this = p_obj->this;	
	
    uint8_t ucTemp;
    ucTemp = (1<<7) | (1<<6) | (addr<<1) | (1<<0); 		//默认 | RAM | 地址 | 读
    ucTemp = ds1302_read_register(p_obj, ucTemp);   	//ram
    *data = ucTemp;

    return R_OK;
}

static int ds1302_set_ram(const c_ds1302* p_obj, uint8_t addr, uint8_t data)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	if(addr > DS1302_RAM_MAX_ADDR){
        return R_PARAM;
    }
	this = p_obj->this;	
	
	uint8_t ucTemp;
	ds1302_write_register(p_obj, 0x8e, 0x00); 		//关闭写保护
    ucTemp = (1<<7) | (1<<6) | (addr<<1) | (0<<0); 	//默认 | RAM | 地址 | 写
    ds1302_write_register(p_obj, ucTemp, data);   	//写ram
    ds1302_write_register(p_obj, 0x8e, 0x80); 		//打开写保护
	
    return R_OK;
}

static int ds1302_get_time(const c_ds1302* p_obj, sys_time_t *time)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this || NULL == time){
        return R_NULL;
    }
	this = p_obj->this;	
	
    uint8_t ntemp;
    ntemp = ds1302_read_register(p_obj, 0x81);   //读秒
    time->sec = BCD_TO_DEC(ntemp);
	
    ntemp = ds1302_read_register(p_obj, 0x83);   //读分
    time->min = BCD_TO_DEC(ntemp);
	
    ntemp = ds1302_read_register(p_obj, 0x85);   //读时
    time->hour = BCD_TO_DEC(ntemp);
	
    ntemp = ds1302_read_register(p_obj, 0x87);   //读天
    time->date = BCD_TO_DEC(ntemp);
	
    ntemp = ds1302_read_register(p_obj, 0x89);   //读月
    time->mon = BCD_TO_DEC(ntemp);
	
	ntemp = ds1302_read_register(p_obj, 0x8b);   //读星期
	time->weekday = BCD_TO_DEC(ntemp);
	
    ntemp = ds1302_read_register(p_obj, 0x8d);   //读年
    time->year = BCD_TO_DEC(ntemp);
    time->year += 2000;

    return R_OK;
}

static int ds1302_set_time(const c_ds1302* p_obj, sys_time_t *time)
{
	s_ds1302* this = NULL;

    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this || NULL == time){
        return R_NULL;
    }
	this = p_obj->this;	
	
    if(time->year < 2000){
        return R_ERROR; 
    }
	
    ds1302_write_register(p_obj, 0x8e, 0x00); 	//关闭写保护
    ds1302_write_register(p_obj, 0x80, DEC_TO_BCD(time->sec)); 			//  秒
    ds1302_write_register(p_obj, 0x82, DEC_TO_BCD(time->min)); 			//  分
    ds1302_write_register(p_obj, 0x84, DEC_TO_BCD(time->hour)); 		//  时
    ds1302_write_register(p_obj, 0x86, DEC_TO_BCD(time->date)); 		//  日
    ds1302_write_register(p_obj, 0x88, DEC_TO_BCD(time->mon)); 			//  月
	ds1302_write_register(p_obj, 0x8a, DEC_TO_BCD(time->weekday)); 		//  周几
    ds1302_write_register(p_obj, 0x8c, DEC_TO_BCD(time->year - 2000)); 	//  年
    ds1302_write_register(p_obj, 0x8e, 0x80); //打开写保护

    return R_OK;
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/********************************************************************************
参考：https://blog.csdn.net/qq_32761549/article/details/130945606

1.软件驱动
 -串口通讯线由管脚 PD_SCK 和 DOUT 组成，用来输出数据，选择输入通道和增益。
 -当数据输出管脚 DOUT 为高电平时，表明A/D转换器还未准备好输出数据，此时串口时钟输入信号 PD_SCK 应为低电平。
 -当 DOUT 从高电平变低电平后，PD_SCK 应输入 25 至 27 个不等的时钟脉冲。
 -其中第一个时钟脉冲的上升沿将读出输出24位数据的最高位（MSB），直至第24个时钟脉冲完成，从最高位至最低位逐位输出
 -第 25 至 27 个时钟脉冲用来选择下一次 A/D 转换的输入通道和增益。
*******************************************************************************/


