/********************************************************************************
* @file		:mpu6050.c
* @brief	:mpu6050 温度检测传感器驱动
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_MPU6050_ENABLED

/* Private Macros ----------------------------------------------------------*/
#define MPU_PI (3.1416)

#define MPU_GYRO_BASE        	0x43    
#define MPU_ACC_BASE         	0x3B     

//#define MPU_ACCEL_OFFS_REG		0x06	//accel_offs寄存器,可读取版本号,寄存器手册未提到
//#define MPU_PROD_ID_REG			0x0C	//prod id寄存器,在寄存器手册未提到
#define MPU_SELF_TESTX_REG		0x0D	//自检寄存器X
#define MPU_SELF_TESTY_REG		0x0E	//自检寄存器Y
#define MPU_SELF_TESTZ_REG		0x0F	//自检寄存器Z
#define MPU_SELF_TESTA_REG		0x10	//自检寄存器A
#define MPU_SAMPLE_RATE_REG		0x19	//采样频率分频器
#define MPU_CFG_REG				0x1A	//配置寄存器
#define MPU_GYRO_CFG_REG		0x1B	//陀螺仪配置寄存器
#define MPU_ACCEL_CFG_REG		0x1C	//加速度计配置寄存器
#define MPU_MOTION_DET_REG		0x1F	//运动检测阀值设置寄存器
#define MPU_FIFO_EN_REG			0x23	//FIFO使能寄存器
#define MPU_I2CMST_CTRL_REG		0x24	//IIC主机控制寄存器
#define MPU_I2CSLV0_ADDR_REG	0x25	//IIC从机0器件地址寄存器
#define MPU_I2CSLV0_REG			0x26	//IIC从机0数据地址寄存器
#define MPU_I2CSLV0_CTRL_REG	0x27	//IIC从机0控制寄存器
#define MPU_I2CSLV1_ADDR_REG	0x28	//IIC从机1器件地址寄存器
#define MPU_I2CSLV1_REG			0x29	//IIC从机1数据地址寄存器
#define MPU_I2CSLV1_CTRL_REG	0x2A	//IIC从机1控制寄存器
#define MPU_I2CSLV2_ADDR_REG	0x2B	//IIC从机2器件地址寄存器
#define MPU_I2CSLV2_REG			0x2C	//IIC从机2数据地址寄存器
#define MPU_I2CSLV2_CTRL_REG	0x2D	//IIC从机2控制寄存器
#define MPU_I2CSLV3_ADDR_REG	0x2E	//IIC从机3器件地址寄存器
#define MPU_I2CSLV3_REG			0x2F	//IIC从机3数据地址寄存器
#define MPU_I2CSLV3_CTRL_REG	0x30	//IIC从机3控制寄存器
#define MPU_I2CSLV4_ADDR_REG	0x31	//IIC从机4器件地址寄存器
#define MPU_I2CSLV4_REG			0x32	//IIC从机4数据地址寄存器
#define MPU_I2CSLV4_DO_REG		0x33	//IIC从机4写数据寄存器
#define MPU_I2CSLV4_CTRL_REG	0x34	//IIC从机4控制寄存器
#define MPU_I2CSLV4_DI_REG		0x35	//IIC从机4读数据寄存器

#define MPU_I2CMST_STA_REG		0x36	//IIC主机状态寄存器
#define MPU_INTBP_CFG_REG		0x37	//中断/旁路设置寄存器
#define MPU_INT_EN_REG			0x38	//中断使能寄存器
#define MPU_INT_STA_REG			0x3A	//中断状态寄存器

#define MPU_ACCEL_XOUTH_REG		0x3B	//加速度值,X轴高8位寄存器
#define MPU_ACCEL_XOUTL_REG		0x3C	//加速度值,X轴低8位寄存器
#define MPU_ACCEL_YOUTH_REG		0x3D	//加速度值,Y轴高8位寄存器
#define MPU_ACCEL_YOUTL_REG		0x3E	//加速度值,Y轴低8位寄存器
#define MPU_ACCEL_ZOUTH_REG		0x3F	//加速度值,Z轴高8位寄存器
#define MPU_ACCEL_ZOUTL_REG		0x40	//加速度值,Z轴低8位寄存器

#define MPU_TEMP_OUTH_REG		0x41	//温度值高八位寄存器
#define MPU_TEMP_OUTL_REG		0x42	//温度值低8位寄存器

#define MPU_GYRO_XOUTH_REG		0x43	//陀螺仪值,X轴高8位寄存器
#define MPU_GYRO_XOUTL_REG		0x44	//陀螺仪值,X轴低8位寄存器
#define MPU_GYRO_YOUTH_REG		0x45	//陀螺仪值,Y轴高8位寄存器
#define MPU_GYRO_YOUTL_REG		0x46	//陀螺仪值,Y轴低8位寄存器
#define MPU_GYRO_ZOUTH_REG		0x47	//陀螺仪值,Z轴高8位寄存器
#define MPU_GYRO_ZOUTL_REG		0x48	//陀螺仪值,Z轴低8位寄存器

#define MPU_I2CSLV0_DO_REG		0x63	//IIC从机0数据寄存器
#define MPU_I2CSLV1_DO_REG		0x64	//IIC从机1数据寄存器
#define MPU_I2CSLV2_DO_REG		0x65	//IIC从机2数据寄存器
#define MPU_I2CSLV3_DO_REG		0x66	//IIC从机3数据寄存器

#define MPU_I2CMST_DELAY_REG	0x67	//IIC主机延时管理寄存器
#define MPU_SIGPATH_RST_REG		0x68	//信号通道复位寄存器
#define MPU_MDETECT_CTRL_REG	0x69	//运动检测控制寄存器
#define MPU_USER_CTRL_REG		0x6A	//用户控制寄存器
#define MPU_PWR_MGMT1_REG		0x6B	//电源管理寄存器1
#define MPU_PWR_MGMT2_REG		0x6C	//电源管理寄存器2 
#define MPU_FIFO_CNTH_REG		0x72	//FIFO计数寄存器高八位
#define MPU_FIFO_CNTL_REG		0x73	//FIFO计数寄存器低八位
#define MPU_FIFO_RW_REG			0x74	//FIFO读写寄存器
#define MPU_DEVICE_ID_REG		0x75	//器件ID寄存器
/* Private Typedef  -------------------------------------------------------------------*/

typedef struct _S_MPU6050
{
	c_my_iic* iic;
}s_mpu6050;

typedef struct _MPU6050_CFG
{
	uint8_t reg;
	uint8_t val;
}mpu6050_cfg_t;

const mpu6050_cfg_t mpu_cfg[] = 
{
	{MPU_PWR_MGMT1_REG, 0x80},		//复位MPU6050
	{MPU_PWR_MGMT1_REG, 0X00},		//唤醒MPU6050	
	{MPU_GYRO_CFG_REG,  (GYRO_RANGE<<3)},	//设置陀螺仪满量程范围 fsr: 0,±250 °/s; 	1,±500 °/s; 	2,±1000 °/s; 	3,±2000 °/s	这里配置寄存器是第 3-4 bit 需要左移3
	{MPU_ACCEL_CFG_REG, (ACC_RANGE<<3)},			//设置加速度满量程范围 fsr: 0,±2g;			1,±4g;			2,±8g;			3,±16g		这里配置寄存器是第 3-4 bit 需要左移3
	{MPU_CFG_REG, 		0x03},		//设置数字低通滤波器  设置采样率50Hz	
	{MPU_INT_EN_REG, 	0x00},		//关闭所有中断	
	{MPU_USER_CTRL_REG, 0x00},		//I2C主模式关闭	
	{MPU_FIFO_EN_REG, 	0x00},		//关闭FIFO	
	{MPU_INTBP_CFG_REG, 0x00},		//INT引脚低电平有效	
	{MPU_PWR_MGMT1_REG, 0x01},		//设置CLKSEL,PLL X轴为参考	
	{MPU_PWR_MGMT2_REG, 0x00},		//加速度与陀螺仪都工作	
};

const float accel_res[4] 	= {16384.0, 8192.0, 4096.0, 2048.0};	//不同量程对应的加速度分辨率
const float gyro_res[4] 	= {131.0, 65.5, 32.8, 16.4};			//不同量程对应的角加速度分辨率
/* Private Functions Declare ----------------------------------------------------------*/
static int mpu6050_read_id(const c_mpu6050 *p_obj);
static int mpu6050_read_acc(const c_mpu6050 *p_obj, float *facc);
static int mpu6050_read_gyro(const c_mpu6050 *p_obj, float *fgyro);
static int mpu6050_read_angle(const c_mpu6050 *p_obj, float *fangle);

/* Private Variables ------------------------------------------------------------------*/
/* Public Functions -------------------------------------------------------------------*/
c_mpu6050 mpu6050_create(c_my_iic *iic)
{
	c_mpu6050 new = {0};	//新建一个空的GPIO对象		
	int ret;
	/*为 MPU6050 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_mpu6050));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_mpu6050));

	/*初始化 MPU6050 对象的私有成员*/
	((s_mpu6050*)new.this)->iic = iic;

	/*初始化 MPU6050 对象的公有接口*/
	new.read_acc 				= mpu6050_read_acc;
	new.read_gyro 				= mpu6050_read_gyro;
	new.read_angle 				= mpu6050_read_angle;
	
	/*MPU6050 传感器初始化配置*/
	for(uint8_t i = 0; i < sizeof(mpu_cfg)/sizeof(mpu6050_cfg_t); i++){
		ret = iic->write_regs(iic, MPU6050_IIC_ADDR, mpu_cfg[i].reg, &mpu_cfg[i].val, 1);
		if(ret != R_OK)	goto errorHandle;
		if(i == 0)		DelayMs(100); 			//发送第一条复位命令后，在初始化之前要延时一段时间，若没有延时，则断电后再上电数据可能会出错
	}
	
	/*器件ID检查*/
	ret = mpu6050_read_id(&new);
	if(ret != R_OK)		goto errorHandle;		//ID错误：检测不到MPU6050模块，请检查模块与开发板的接线
	
	DelayMs(200);
	return new;

errorHandle:
	new.this = NULL;
	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static int mpu6050_read_id(const c_mpu6050 *p_obj)
{
    s_mpu6050* this = NULL;
	int ret;
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this) {
        return R_NULL;
    }
    this = p_obj->this;

	uint8_t Re = 0;
	ret = this->iic->read_regs(this->iic, MPU6050_IIC_ADDR, MPU_DEVICE_ID_REG, &Re, 1);
	if(ret != R_OK)		return R_ERROR;
	
	if(Re != 0x68)		return R_ERROR;		//检测不到MPU6050模块，请检查模块与开发板的接线
	else				return R_OK;		//0x68 为正确ID, 
}

static int mpu6050_read_acc(const c_mpu6050 *p_obj, float *facc)
{
    s_mpu6050* this = NULL;
	int ret;
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this) {
        return R_NULL;
    }
    this = p_obj->this;

    uint8_t buf[6];
	short xAccData[3];

	this->iic->read_regs(this->iic, MPU6050_IIC_ADDR, MPU_ACC_BASE, buf, 6);		//读取加速度寄存器的值
    xAccData[0] = ((short)buf[0] << 8) | buf[1];	//计算最终值
    xAccData[1] = ((short)buf[2] << 8) | buf[3];
    xAccData[2] = ((short)buf[4] << 8) | buf[5];

	facc[0] = xAccData[0] / accel_res[ACC_RANGE];	//除以分辨率 转换为小数，单位 g
    facc[1] = xAccData[1] / accel_res[ACC_RANGE];
    facc[2] = xAccData[2] / accel_res[ACC_RANGE];

	return R_OK;
}

static int mpu6050_read_gyro(const c_mpu6050 *p_obj, float *fgyro)
{
    s_mpu6050* this = NULL;
	int ret;
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this){
        return R_NULL;
    }
    this = p_obj->this;

    uint8_t buf[6];
	short xGyroData[3];

	this->iic->read_regs(this->iic, MPU6050_IIC_ADDR, MPU_GYRO_BASE, buf, 6);		//读取加速度寄存器的值
    xGyroData[0] = ((short)buf[0] << 8) | buf[1];	//计算最终值
    xGyroData[1] = ((short)buf[2] << 8) | buf[3];
    xGyroData[2] = ((short)buf[4] << 8) | buf[5];

	fgyro[0] = xGyroData[0] / gyro_res[GYRO_RANGE];	//转换为小数，单位dps (°/s)
    fgyro[1] = xGyroData[1] / gyro_res[GYRO_RANGE];
    fgyro[2] = xGyroData[2] / gyro_res[GYRO_RANGE];
	
	return R_OK;
}

static int mpu6050_read_angle(const c_mpu6050 *p_obj, float *fangle)
{
    s_mpu6050* this = NULL;
	int ret;
    /*检查参数*/
    if(NULL == p_obj || NULL == p_obj->this) {
        return R_NULL;
    }
    this = p_obj->this;

    uint8_t buf[6];
	float facc[3];
	short xAccData[3];

	this->iic->read_regs(this->iic, MPU6050_IIC_ADDR, MPU_ACC_BASE, buf, 6);		//读取加速度寄存器的值
    xAccData[0] = ((short)buf[0] << 8) | buf[1];	//计算最终值
    xAccData[1] = ((short)buf[2] << 8) | buf[3];
    xAccData[2] = ((short)buf[4] << 8) | buf[5];

	facc[0] = xAccData[0] / accel_res[ACC_RANGE];	//除以分辨率 转换为小数，单位 g
    facc[1] = xAccData[1] / accel_res[ACC_RANGE];
    facc[2] = xAccData[2] / accel_res[ACC_RANGE];

	fangle[0] = atan(facc[0]/sqrt(facc[1]*facc[1] + facc[2]*facc[2])) * 180 / MPU_PI;
    fangle[1] = atan(facc[1]/sqrt(facc[0]*facc[0] + facc[2]*facc[2])) * 180 / MPU_PI;
    fangle[2] = atan(facc[2]/sqrt(facc[0]*facc[0] + facc[1]*facc[1])) * 180 / MPU_PI;

	return R_OK;
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/********************************************************************************
参考：野火小智 MPU6050 温湿度模块 http://www.firebbs.cn

1. 串行通信说明（单线双向）
 -单总线是一种通信方式，只使用一根数据线进行数据交换和控制，下面对其传输数据构成以及通信时序进行介绍
 -一次传送 40 位数据，共 5 字节，高位先出
 -数据格式：8bit 湿度整数 + 8bit 湿度小数 + 8bit 温度整数 + 8bit 温度小数+ 8bit 校验和（注：其中湿度小数为0）
 -数据传送正确时，校验和等于四字节数据之和：校验和 = 湿度整数 +湿度小数 +温度整数 +温度小数。

2. 通信时序可分为：建立连接、从机响应信号、数据接收三部分

--建立连接：
 -MPU6050 温湿度传感器上电后，数据线处于空闲状态（数据线由上拉电阻拉高一直保持高电平）
 -主机发送起始信号：主机的 I/O 设置为输出状态，拉低数据线（数据线拉低时间＞18ms），然后主机的 I/O 设置为输入状态，
 -释放总线，由于上拉电阻，数据线也随之变高电平

--从机响应信号：
 -从机接收到主机发送的起始信号后，先拉低数据线，再拉高数据线，表示连接建立成功，通知主机准备接收数据（拉低数据线 83us，拉高数据线 87us）

--数据接收：
 -从机发送 40 位数据，每 1bit 数据都以 54us 低电平开始，主机通过判断低电平后的高电平时间来决定接收数据的种类
 -位数据“0” 的格式为： 54 us 的低电平和 23-27 us 的高电平
 -位数据“1”的格式为： 54 us 的低电平加 68-74 us 的高电平
 -当最后 1 bit 数据传送完毕后，从机拉低数据线 54us，随后从机释放总线，总线进入空闲状态（由上拉电阻拉高变为高电平）
*******************************************************************************/


