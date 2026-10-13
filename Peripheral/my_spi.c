/********************************************************************************
* @file		:my_spi.c
* @brief	:1.STM32 HAL 库中 SPI 的二次封装；2.模拟 SPI 的封装
* @Author	:Gwen9
* @Date		:2024/08/04
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_SPI_ENABLED

/* Private Macros  ------------------------------------------------------------------*/
#define	DEFAULT_SPEED	1	//spi 默认速度

#define CS_HIGH()		HAL_GPIO_WritePin(this->cs_gpio, this->cs_pin, GPIO_PIN_SET)
#define CS_LOW()		HAL_GPIO_WritePin(this->cs_gpio, this->cs_pin, GPIO_PIN_RESET)

#define SCK_HIGH()		HAL_GPIO_WritePin(this->sck_gpio, this->sck_pin, GPIO_PIN_SET)
#define SCK_LOW()		HAL_GPIO_WritePin(this->sck_gpio, this->sck_pin, GPIO_PIN_RESET)

#define MOSI_HIGH()		HAL_GPIO_WritePin(this->mosi_gpio, this->mosi_pin, GPIO_PIN_SET)
#define MOSI_LOW()		HAL_GPIO_WritePin(this->mosi_gpio, this->mosi_pin, GPIO_PIN_RESET)
	
#define MISO_READ()		HAL_GPIO_ReadPin(this->miso_gpio, this->miso_pin)	

#define SPI_DELAY(n)	DelayUs(n*(this->speed))

/* Private Typedef  -------------------------------------------------------------------*/
/*IIC 对象 私有成员*/
typedef struct _S_MY_SPI
{
    GPIO_TypeDef*      	cs_gpio;  	/*CS/NSS*/
    uint16_t           	cs_pin;   	
    GPIO_TypeDef*      	sck_gpio;  	/*SCK*/
    uint16_t           	sck_pin;   	
    GPIO_TypeDef*      	mosi_gpio;  /*MOSI*/
    uint16_t           	mosi_pin;   
    GPIO_TypeDef*      	miso_gpio;  /*MISO*/
    uint16_t           	miso_pin;   
	uint32_t			speed;	    /*模拟 SPI 的速度*/
}s_my_spi;

/* Private Functions Declare ----------------------------------------------------------*/
static int my_spi_send_bytes(const c_my_spi *p_obj, uint8_t* txData, uint16_t len);
static int my_spi_read_bytes(const c_my_spi *p_obj, uint8_t* rxData, uint16_t len);
static int my_spi_transfer_bytes(const c_my_spi *p_obj, uint8_t* txData, uint8_t* rxData, uint16_t len);
static int my_spi_set_speed(const c_my_spi *p_obj, uint32_t speed);

/* Private Variables -------------------------------------------------------------------*/
//const c_my_spi_t my_spi;

/* Public Functions -------------------------------------------------------------------*/
c_my_spi my_spi_creat(GPIO_TypeDef* cs_gpio, uint16_t cs_pin, GPIO_TypeDef* sck_gpio, uint16_t sck_pin, GPIO_TypeDef* mosi_gpio, uint16_t mosi_pin, GPIO_TypeDef* miso_gpio, uint16_t miso_pin)
{
	c_my_spi new = {0};					/*新建一个空的 IIC 对象*/	
	
	/*为 IIC 新对象的私有成员申请内存*/
	new.this = MY_MALLOC(sizeof(s_my_spi));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_my_spi));

	/*初始化GPIO对象的私有成员*/
	((s_my_spi*)new.this)->sck_gpio 	= sck_gpio;
	((s_my_spi*)new.this)->sck_pin 		= sck_pin;
	((s_my_spi*)new.this)->mosi_gpio 	= mosi_gpio;
	((s_my_spi*)new.this)->mosi_pin 	= mosi_pin;
	((s_my_spi*)new.this)->miso_gpio 	= miso_gpio;
	((s_my_spi*)new.this)->miso_pin 	= miso_pin;
	((s_my_spi*)new.this)->cs_gpio 		= cs_gpio;
	((s_my_spi*)new.this)->cs_pin 		= cs_pin;

	((s_my_spi*)new.this)->speed		= DEFAULT_SPEED;
	
	/*初始化GPIO对象的公有接口*/
	new.read_bytes 		= my_spi_read_bytes;
	new.send_bytes 		= my_spi_send_bytes;
	new.transfer_bytes	= my_spi_transfer_bytes;
	new.set_speed 		= my_spi_set_speed;

	/*初始化对应的GPIO*/
	my_gpio.init(sck_gpio, 	sck_pin, 	GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	my_gpio.init(mosi_gpio, mosi_pin, 	GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	my_gpio.init(miso_gpio, miso_pin, 	GPIO_MODE_INPUT, 	 GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	my_gpio.init(cs_gpio, 	cs_pin, 	GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
	
	/*拉高片选释放设备*/
	HAL_GPIO_WritePin(cs_gpio, cs_pin, GPIO_PIN_SET);
	
	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static void delay(uint32_t x)
{
	for (volatile int i = 0; i < x; i++);
}

static int my_spi_send_bytes(const c_my_spi *p_obj, uint8_t* txData, uint16_t len)
{
	const s_my_spi* this = NULL;
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	
	/*设置指针*/
	this = p_obj->this;

	/*拉低片选选中设备*/
	CS_LOW();
	
	/*连续数据发送*/ 
	for(int j = 0; j < len; j++){
		for(int i = 0; i < 8; i++)
		{	
			SCK_LOW();							//时钟线，空闲为低，CPOL = 0
			SPI_DELAY(1);						
			if(txData[j]&0x80)	MOSI_HIGH();	//主设备通过 MOSI 发送一位数据
			else				MOSI_LOW();					
			SCK_HIGH();							//上升沿信号通知设备读取
			SPI_DELAY(1);						//等待设备读取完成
			txData[j] <<= 1;
		}
		SCK_LOW();	//时钟线，空闲为低，CPOL = 0
	}

	/*拉高片选释放设备*/
	CS_HIGH();
	
	/*返回结果*/
	return	R_OK;
}

static int my_spi_read_bytes(const c_my_spi *p_obj, uint8_t* rxData, uint16_t len)
{
	const s_my_spi* this = NULL;
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	this = p_obj->this;

	/*拉低片选选中设备*/
	CS_LOW();
	/*连续数据读取*/ 
	for(int j = 0; j < len; j++){
		SCK_LOW();						//时钟线，空闲为低，CPOL = 0
		for(int i = 0; i < 8; i++)
		{
			SCK_HIGH();					
			rxData[j] <<= 1;
			rxData[j]  |= MISO_READ();	//读取从设备发送的数据
			SCK_LOW();					//下降沿信号 通知从设备发送数据
			SPI_DELAY(5);				//等待从设备 发送数据完成(准备好数据)
		}
		
//		SCK_LOW();						//时钟线，空闲为低，CPOL = 0
//		for(int i = 0; i < 8; i++)
//		{
//			SCK_HIGH();											
//			SPI_DELAY(1);
//			SCK_LOW();					//下降沿信号 通知从设备发送数据
//			SPI_DELAY(5);				//等待从设备 发送数据完成(准备好数据)
//			rxData[j] <<= 1;
//			rxData[j]  |= MISO_READ();	//读取从设备发送的数据
//		}
	}

	/*拉高片选释放设备*/
	CS_HIGH();
	/*返回结果*/
	return	R_OK;
}

static int my_spi_transfer_bytes(const c_my_spi *p_obj, uint8_t* txData, uint8_t* rxData, uint16_t len)
{
	const s_my_spi* this = NULL;
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	this = p_obj->this;
	
	/*拉低片选选中设备*/
	CS_LOW();
	/*连续数据转换*/
	for(int j = 0; j < len; j++){
		for (int i = 0; i < 8; i++){
			if(txData[j] & 0x80) MOSI_HIGH();
			else				 MOSI_LOW();
			txData[j] <<= 1;

			SCK_HIGH();
			rxData[j] <<= 1;
			if(MISO_READ())		 rxData[j] |= 0x01;
			SCK_LOW();
		}
	}
	/*拉高片选释放设备*/
	CS_HIGH();
	/*返回结果*/
	return	R_OK;
}

static int my_spi_set_speed(const c_my_spi *p_obj, uint32_t speed)
{
	s_my_spi* this = NULL; 
	
	/*参数检测*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
	this = p_obj->this;

	this->speed = speed;
	return R_OK;
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/*
1.工作模式
SPI 根据时钟极性和时钟相位的不同可以有4种工作模式，通常在编写 SPI 设备驱动程序时需要特别注意从设备的时钟相位与时钟极性，
如果与主设备（SOC）不一致时，一般都不能进行正常的通信。
-- 时钟极性(CPOL)指通讯设备处于空闲状态( SPI 开始通讯前、nSS 线无效)时 SCK 的状态。
    CPOL = 0：SCK在空闲时为低电平
    CPOL = 1：SCK在空闲时为高电平
-- 时钟相位(CPHA)指数据的采样时刻位于 SCK 的偶数边沿采样还是奇数边沿采样。
    CPHA = 0：在SCK的奇数边沿采样
    CPHA = 1：在SCK的偶数边沿采样
-- 通过时钟极性和时钟相位的不同组合 SPI 总共可以设置为4种工作模式
MODE0 : CPOL = 0    CPHA = 0    SCK空闲为低，SCK的奇数次边沿采样    
MODE1 : CPOL = 0    CPHA = 1    SCK空闲为低，SCK的偶数次边沿采样    
MODE2 : CPOL = 1    CPHA = 0    SCK空闲为高，SCK的奇数次边沿采样    
MODE3 : CPOL = 1    CPHA = 1    SCK空闲为高，SCK的偶数次边沿采样   

2. SPI时序分析实例
SPI 通讯时 nSS 、SCK 、MOSI 信号均由主机产生，MISO 信号由从机产生。在 nSS 为低电平（片选选中）的前提下，
MOSI 和 MISO 信号才有效。在每个时钟周期 MOSI 和 MISO 传输一位数据。跟I2C通讯类似，SPI通讯也需要通讯的起始/结束信号，有效数据 和 同步时钟。
起始/结束信号：
nSS 信号由高电平变为低电平即为 SPI 通讯的起始信号，反过来， nSS 信号由低电平变为高电平即为SPI通讯的结束信号。
当从机检测到自身的 nSS 引脚被拉低时就知道自己被主机选中，准备和主机进行通讯。
数据同步：
SCK 用于数据同步，MOSI 和 MISO 线上的数据在每个 SCK 时钟周期传输一位数据，数据的输入/输出可以同时进行(双线全双工)。

3. 至此就可以大致归纳出模拟该设备SPI通信的编程思路：
1.拉低片选信号
2.延时 至少 t_cs 拉低时钟线
3.延时 至少 t_sl 后，主设备立即将需要发送的电平信号发送待 MOSI 上
4.再拉高时钟线，主设备读 MISO 的电平信号，时钟高维持 t_sh 时间后周期结束
5.垃低时钟线，重复步骤3、4开始读取下一位的数据
7.最后一位数据读取完成，拉高片选
注意：需要保证交换一位数据的周期时间大于 200ns

4. 注意：理解时钟相位只需要明白一点，无论我们的编程对象是主机还是从机只需要记住在采样边沿发生之前，
应该准备好我们当前设备需要发送的数据在采样边沿发生之后，再去读取其他设备发送过来的数据。

参考：https://www.cnblogs.com/Gentle-Wen/p/13639965.html

5. HAL 库硬件SPI相关函数
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size,
                                          uint32_t Timeout);

1. 标准SPI
标准SPI通常就称SPI，它是一种串行外设接口规范，有4根引脚信号：clk , cs, mosi, miso
2. Dual SPI
它只是针对SPI Flash而言，不是针对所有SPI外设。对于SPI Flash，全双工并不常用，因此扩展了mosi和miso的用法，让它们工作在半双工，
用以加倍数据传输。也就是对于Dual SPI Flash，可以发送一个命令字节进入dual mode，这样mosi变成SIO0（serial io 0），
mosi变成SIO1（serial io 1）,这样一个时钟周期内就能传输2个bit数据，加倍了数据传输
3. Qual SPI
与Dual SPI类似，也是针对SPI Flash，Qual SPI Flash增加了两根I/O线（SIO2,SIO3），目的是一个时钟内传输4个bit所以对于SPI Flash，
有标准spi flash，dual spi , qual spi 三种类型，分别对应3-wire, 4-wire, 6-wire，在相同clock下，线数越多，传输速率越高。
btw：spi flash一般为NOR Flash
*/

