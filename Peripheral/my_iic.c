/********************************************************************************
* @file		:my_iic.c
* @brief	:对STM32 HAL 库中 IIC 的二次封装，以及模拟IIC的封装
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_IIC_ENABLED

/* Private Macros  ------------------------------------------------------------------*/
#define	DEFAULT_SPEED			1		//iic 默认速度 延时1us
#define DEFAULT_ACK_CHECK		true	//默认需要校验ack
#define DEFAULT_ACK_OVERTIME	80		//默认ack校验超时时间 单位us

#define SCL_HIGH()		HAL_GPIO_WritePin(this->scl_gpio, this->scl_pin, GPIO_PIN_SET)
#define SCL_LOW()		HAL_GPIO_WritePin(this->scl_gpio, this->scl_pin, GPIO_PIN_RESET)

#define SDA_HIGH()		HAL_GPIO_WritePin(this->sda_gpio, this->sda_pin, GPIO_PIN_SET)
#define SDA_LOW()		HAL_GPIO_WritePin(this->sda_gpio, this->sda_pin, GPIO_PIN_RESET)
	
#define SDA_READ()		HAL_GPIO_ReadPin(this->sda_gpio, this->sda_pin)	

#define IIC_DELAY()		DelayUs(this->speed)
/* Private Typedef  -------------------------------------------------------------------*/
/*IIC 对象 私有成员*/
typedef struct _S_MY_IIC
{
    GPIO_TypeDef*      	scl_gpio;  		/*scl gpio   */
    uint16_t           	scl_pin;   		/*scl  pin   */
    GPIO_TypeDef*      	sda_gpio;  		/*sda gpio   */
    uint16_t           	sda_pin;   		/*sda  pin   */
	uint32_t			speed;	   		/*模拟iic的速度*/
	bool				ack_check;		/*是否需要校验ack*/
	uint16_t			ack_wait_time;	/*ack 校验超时时间*/
}s_my_iic;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_iic_write_regs(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint8_t len);
static int my_iic_read_regs(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t len);
static int my_iic_set_param(const c_my_iic *p_obj, uint32_t speed, bool ack_check, uint32_t ack_wait_time);

static int my_iic_send_byte(const c_my_iic *p_obj, uint8_t data);
static uint8_t my_iic_recive_byte(const c_my_iic *p_obj, bool ack);
static int my_iic_start(const c_my_iic *p_obj);
static int my_iic_stop(const c_my_iic *p_obj);

/* Private Variables -------------------------------------------------------------------*/

/* Public Functions -------------------------------------------------------------------*/
c_my_iic my_iic_create(GPIO_TypeDef* scl_gpio, uint16_t scl_pin, GPIO_TypeDef* sda_gpio, uint16_t sda_pin)
{
	c_my_iic new = {0};					/*新建一个空的 IIC 对象*/	
	
	/*为 IIC 新对象的私有成员申请内存*/
	new.this = MY_MALLOC(sizeof(s_my_iic));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_my_iic));

	/*初始化GPIO对象的私有成员*/
	((s_my_iic*)new.this)->scl_gpio 		= scl_gpio;
	((s_my_iic*)new.this)->scl_pin 			= scl_pin;
	((s_my_iic*)new.this)->sda_gpio 		= sda_gpio;
	((s_my_iic*)new.this)->sda_pin 			= sda_pin;
	((s_my_iic*)new.this)->speed			= DEFAULT_SPEED;
	((s_my_iic*)new.this)->ack_check		= DEFAULT_ACK_CHECK;
	((s_my_iic*)new.this)->ack_wait_time	= DEFAULT_ACK_OVERTIME;
	
	/*初始化GPIO对象的公有接口*/
	new.write_regs 	= my_iic_write_regs;
	new.read_regs 	= my_iic_read_regs;
	new.set_param 	= my_iic_set_param;
	new.send_byte	= my_iic_send_byte;
	new.recive_byte	= my_iic_recive_byte;
	new.start		= my_iic_start;
	new.stop		= my_iic_stop;
	
	/*初始化对应的GPIO*/
	my_gpio.init(scl_gpio, scl_pin, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	my_gpio.init(sda_gpio, sda_pin, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);

	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static int my_iic_start(const c_my_iic *p_obj)
{
	const s_my_iic* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;
	
	/*发送开始信号*/
	SDA_HIGH();		//拉高 SDA
	IIC_DELAY();	//++
	SCL_HIGH();		//拉高 SCL
	IIC_DELAY();	//延时等待总线上的电平稳定
	SDA_LOW();		//SDA由高向低跳变
	IIC_DELAY();	//时序图中的 t4，开始或重新开始总线时需要维持的时间
	SCL_LOW();	 	//拉低时钟线，为后续数据发送做准备    
	IIC_DELAY();

	/*返回结果*/
	return	R_OK;
}

static int my_iic_stop(const c_my_iic *p_obj)
{
	const s_my_iic* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;

	/*发送停止信号*/
	SDA_LOW();      //拉低数据线  
	IIC_DELAY();   //这里的延时是为了等待总线上的电平稳定
	SCL_HIGH();     //拉高时钟线                  
	IIC_DELAY();   //这里的延时是为了等待总线上的电平稳定
	SDA_HIGH();     //SDA由低向高跳变
	IIC_DELAY(); 	//时序图中的 t8，总线停止时需要维持的时间
  	
	/*返回结果*/
	return	R_OK;
}

static int my_iic_send_byte(const c_my_iic *p_obj, uint8_t data)
{
	const s_my_iic* this = NULL; 
	bool ack;
	uint16_t ack_count = 0;

	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;

	/*循环发送1byte的数据，先发送高位（MSB），SDA线上呈现需要发送的数据电平。*/                                 
	for(unsigned char cnt = 0; cnt < 8; cnt++){ 
		if(data & 0x80) 	SDA_HIGH();                                                        
		else            	SDA_LOW();   
		IIC_DELAY();  	//等待SDA线电平稳定  
		SCL_HIGH();    	//拉高SCL，SDA上的数据有效
		IIC_DELAY(); 	//从机检测到SCL为高时读走SDA上的一位数据,这里延时等待从机读取
		SCL_LOW();   	//拉低SCL, SDA上的数据无效，1位数据发送完成
		IIC_DELAY();	
		data <<= 1;   	//数据移位发送下一位数据   
	}
	
	/*读取 应答/非应答 信号*/
	SCL_LOW();    	//拉低SCL，SDA上的数据无效                                                           
	IIC_DELAY(); 	//等待SDA线的电平稳定
	SDA_HIGH();   	//释放SDA，将SDA的控制权交给从机  
	IIC_DELAY(); 	//等待SDA线的电平稳定
	SCL_HIGH();   	//拉高SCL，SDA上的数据有效，告诉从机需要发送响应                                                      
	IIC_DELAY();  	//等待从机将响应信号发送到SDA线上

	do{
		if(SDA_READ())  ack = false;  	//读取SDA线上的响应 NOACK
		else            ack = true;    	//读取SDA线上的响应 ACK	
		DelayUs(1);
		ack_count++;
	}while(ack == false && ack_count < this->ack_wait_time);

	SCL_LOW(); 		//拉低SCL，SDA上的数据无线,应答信号读取                                                         
	IIC_DELAY();
	
	/*如果需要校验ack 且ack无效时返回通信失败*/
	if(this->ack_check && ack == false)		return R_ERROR;

	/*返回结果*/
	return	R_OK;
}

static uint8_t my_iic_recive_byte(const c_my_iic *p_obj, bool ack)
{
	const s_my_iic* this = NULL;
	uint8_t data = 0;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;
	
  	/*释放SDA(SDA配置为开漏输出，拉高即可以释放总线，将控制权交给从机)*/
	SDA_HIGH(); 	
	
	/*循环读取1byte 的数据，先读取高位（MSB）*/
	for(unsigned char cnt = 0; cnt < 8; cnt++){
		data <<= 1;
		IIC_DELAY();    
		SCL_HIGH(); 					//主机拉高SCL
		IIC_DELAY();   					//SCL拉高之后从机会在总线上准备好需要发送的数据，这里延时等待
		if(SDA_READ())	data |= 0x01;	//读取SDA线上的数据
		SCL_LOW(); 						//拉低SCL, SDA上的数据无效，1位数据读取完成
		IIC_DELAY();        
	}
	
	/*发送 应答/非应答 信号*/
	SCL_LOW();            	//拉低SCL，SDA上的数据无效                                                   
	IIC_DELAY();        	//等待SDA线的电平稳定
	
	if(ack) SDA_LOW();    	//发送一位数据（ACK/NACK）                                          
	else    SDA_HIGH(); 
								 
	IIC_DELAY();      		//等待SDA线电平稳定
	SCL_HIGH();       		//拉高SCL，SDA上的数据有效                                               
	IIC_DELAY();   			//延时等待从机读取
	SCL_LOW();    			//拉低SCL，SDA上的数据无线,应答信号发送完成                                               
	IIC_DELAY();

	/*返回结果*/
	return data;   
}

static int my_iic_write_regs(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint8_t len)
{
	int ret;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	//发送起始信号
	my_iic_start(p_obj);
	//发送设备地址+写命令
	ret = my_iic_send_byte(p_obj, (dev_addr<<1)|IIC_CMD_WR); 	if(ret != R_OK) return R_ERROR;
	//写寄存器地址	
	ret = my_iic_send_byte(p_obj, reg_addr);					if(ret != R_OK) return R_ERROR;					
	//发送数据并等待相应
	for(uint8_t cnt = 0; cnt < len; cnt++){
		ret = my_iic_send_byte(p_obj, data[cnt]);				if(ret != R_OK) return R_ERROR;	
	}
	//发送停止信号
 	my_iic_stop(p_obj);
	
	return R_OK;
}

static int my_iic_read_regs(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t len)
{
	/*参数检测*/
	int ret;
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	//发送起始信号
	my_iic_start(p_obj);
	
	//发送设备地址 + 写命令	
	ret = my_iic_send_byte(p_obj, (dev_addr<<1)|IIC_CMD_WR); 	if(ret != R_OK) return R_ERROR;
	//写寄存器地址
	ret = my_iic_send_byte(p_obj, reg_addr);					if(ret != R_OK) return R_ERROR;

	//重启IIC总线
	my_iic_start(p_obj);
	//发送设备地址 + 读命令
	ret = my_iic_send_byte(p_obj, (dev_addr<<1)|IIC_CMD_RD); 	if(ret != R_OK) return R_ERROR;	
	
	while(len){
		if(len == 1){	//最后一字节数据
           *data = my_iic_recive_byte(p_obj, false);	//读一字节数据，读完发送 NACK
        }else {
           *data = my_iic_recive_byte(p_obj, true);	//读一字节数据，读完发送 ACK
        } 
		len--;
		data++; 
	}  
	
 	my_iic_stop(p_obj);						//发送停止信号
	
	return R_OK;
}

static int my_iic_set_param(const c_my_iic *p_obj, uint32_t speed, bool ack_check, uint32_t ack_wait_time)
{
	s_my_iic* this = NULL; 
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;
	
	this->speed 		= speed;
	this->ack_check		= ack_check;
	this->ack_wait_time	= ack_wait_time;

	return R_OK;
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/*
SCL为时钟线，SDA为数据线，SCL和SDA默认都是高电平，两条线相互配合会产生三种信号构成时序。

开始信号：SCL 为高电平时，SDA 由高电平向低电平跳变。

结束信号：SCL 为高电平时，SDA 由低电平向高电平跳变。

应答信号：接收数据的设备在接收到 8位 数据后，向发送数据的设备发出特定的低电平，表示已收到数据。
主机设备向从机设备发出一个信号后，等待从机设备发出一个应答信号，主机设备接收到应答信号后，
根据实际情况作出是否继续传递信号。若未收到应答信号，由判断为受控单元出现故障

IIC在开始信号发出后开始发送数据，数据以8位传输，SCL高电平的时候SDA读到的数据有效，
然后经历8位数据传输以后，第九次检测应答信号，如果检测到从机将SDA置为低电平，
说明从机设备有应答（ACK），如果保持高电平，说明从机设备没有应答（NACK）。

https://blog.csdn.net/qq_38072731/article/details/140242166

https://blog.csdn.net/qq_43278452/article/details/126251103

STM32 硬件IIC BUSY问题：
https://blog.csdn.net/qq_34901073/article/details/125682555
*/


