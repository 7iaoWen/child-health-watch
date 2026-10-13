/********************************************************************************
* @file		:max30102.c
* @brief	:max30102 心率血氧检测模块 注：测量时请尽量保持模块和手指相对静止，模块不移动和抖动
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_MAX30102_ENABLED

/* Private Macros ----------------------------------------------------------*/
#define MAX_BRIGHTNESS 			255

#define I2C_WRITE_ADDR 			0xAE
#define I2C_READ_ADDR 			0xAF

#define INTERRUPT_STATUS1 		0X00		// （只读）用于读取中断状态, INTERRUPT_STATUS1 包含中断标志位，详见文末
#define INTERRUPT_STATUS2 		0X01		// （只读）用于读取中断状态, INTERRUPT_STATUS2 包含中断标志位，详见文末
#define INTERRUPT_ENABLE1 		0X02		// 用于启用或禁用中断功能。Bit 7:FIFO 几乎已满中断； Bit 6:PPG 中断； Bit5:ALC 中断 
#define INTERRUPT_ENABLE2 		0X03		// 用于启用或禁用中断功能。Bit 1:芯片内温度传感器就绪中断

#define FIFO_WR_POINTER 		0X04		// FIFO 写指针，用于指示当前数据写入 FIFO 的位置： FIFO_WR_PTR[4:0]
#define FIFO_OV_COUNTER 		0X05		//（只读）FIFO 溢出计数器，当 FIFO 满时，新的数据会覆盖旧数据。该寄存器记录了 FIFO 溢出的次数,用于监控 FIFO 的溢出情况。OVF_this->COUNTER[4:0]
#define FIFO_RD_POINTER 		0X06		// FIFO 读指针，用于指示从 FIFO 读取数据的位置。	FIFO_RD_PTR[4:0]
#define FIFO_DATA 				0X07		// 用于从 FIFO 中读取传感器的输出数据。FIFO_DATA[7:0]

#define FIFO_CONFIGURATION 		0X08		// 配置 FIFO 的数据采集方式，包括采样率、FIFO 的存储深度等，详见文末
#define MODE_CONFIGURATION 		0X09		// 用于配置传感器的工作模式，Bit7:SHDN (Shutdown);  Bit6:RESET;  Bit2-0:0x02（心率模式）,   0x03（SpO2 模式）， 0x07（多LED模式）。
#define SPO2_CONFIGURATION 		0X0A		// 配置 SpO2 模式下的测量参数，如 ADC 分辨率和采样率, 详见文末

#define LED1_PULSE_AMPLITUDE 	0X0C		// 设置红色 LED1 的发射电流级别，即发射 LED 的强度，用于心率和血氧检测, 0x00 关闭 LED，0xFF 设置为最大强度。
#define LED2_PULSE_AMPLITUDE 	0X0D		// 设置红色 LED2 的发射电流级别，即发射 LED 的强度，用于心率和血氧检测, 0x00 关闭 LED，0xFF 设置为最大强度。

#define MULTILED1_MODE 			0X11		// 配置多LED模式下的 LED 使用情况和序列, 可以设置不同的 LED 组合用于多波长测量
#define MULTILED2_MODE 			0X12

#define TEMPERATURE_INTEGER 	0X1F		// 读取传感器的温度数据，其中 TEMPERATURE_INTEGER 记录温度的整数部分，
#define TEMPERATURE_FRACTION 	0X20		// TEMPERATURE_FRACTION 记录小数部分
#define TEMPERATURE_CONFIG 		0X21		//  配置温度传感器的启动和数据采集。设置启动温度测量的命令，0x01:启动温度测量 0x00:不启动温度测量

#define VERSION_ID 				0XFE		// (只读) 设备的版本ID
#define PART_ID 				0XFF		//（只读）设备的部件ID

#define MA4_SIZE  		4 	 // DONOT CHANGE
#define min(x,y) 		((x) < (y) ? (x) : (y))

/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _MAX30102_CFG
{
	uint8_t reg;
	uint8_t val;
}max30102_cfg_t;

typedef struct _S_MAX30102
{
	c_my_iic* 	iic;
	
	uint32_t	*red_buf			; // 红色 LED 传感器数据缓冲区			
	uint32_t	*ir_buf				; // 红外 LED 传感器数据缓冲区	 
	
    int32_t 	n_ir_buffer_length	; // 数据长度
	
	//心率
	int32_t 	n_heart_rate		; // 心率值
	int8_t  	ch_hr_valid			; // 指示心率计算是否有效
    int32_t 	hr_buf[16]			; // 存储心率样本
    int32_t 	hrSum				; // 心率总和	
    int32_t 	hrAvg				; // 心率的平均值
	int32_t 	hrBuffFilled		; // 心率缓冲区填充情况
	int32_t 	hrValidCnt			; // 有效心率计数 			0
	int32_t 	hrThrowOutSamp 		; // 指示是否丢弃心率样本 	0
	int32_t 	hrTimeout 			; // SPO2 超时计数器 		0
	
	//血氧浓度
	int32_t 	n_spo2				; // 计算得到的 SPO2 值
	int8_t 		ch_spo2_valid		; // 指示 SPO2 计算是否有效
    int32_t 	spo2_buf[16]		; // 存储 SPO2 样本
    int32_t 	spo2Sum				; // SPO2 总和
    int32_t 	spo2Avg				; // SPO2 的平均值
    int32_t 	spo2BuffFilled		; // SPO2 缓冲区填充情况
	int32_t 	spo2ValidCnt		; // 有效 SPO2 计数 		0
    int32_t 	spo2ThrowOutSamp 	; // 指示是否丢弃 SPO2 样本 0
    int32_t 	spo2Timeout 		; // 心率超时计数器 		0
	
//	uint32_t 	un_min				; 
//	uint32_t 	un_max				;
	uint32_t 	un_prev_data		; 
	uint32_t 	un_brightness		; // 计算LED亮度的变量，反映心跳
//    int32_t 	i					; // 循环索引
    float 		f_temp				; // 临时浮点变量
	int32_t 	COUNT 				; // 计数器变量 			0
	
	int32_t     s_theHeartRate  	;//心率	计算结果
	int32_t     s_theO2    			;//血氧浓度 计算结果
}s_max30102;

const max30102_cfg_t max_cfg[] = 
{
	{MODE_CONFIGURATION, 	0x40},		//复位	
	{INTERRUPT_ENABLE1, 	0xE0},		//开启中断：FIFO几乎已满中断， PPG中断， ALC中断 （野火为：0xe0）
	{INTERRUPT_ENABLE2,  	0x00},		//不开启中断：芯片内温度传感器就绪中断

	{FIFO_WR_POINTER, 		0x00},		//复位FIFO写指针		
	{FIFO_OV_COUNTER, 		0x00},		//复位FIFO溢出计数
	{FIFO_RD_POINTER, 		0x00},		//复位FIFO读指针

	{FIFO_CONFIGURATION, 	0x6f},		//01001111: 010:平均4个样本; 0:禁用回滚模式; 1111:当 FIFO 还剩下 15 个样本（已写入 17 个样本）时触发中断 
	{MODE_CONFIGURATION, 	0x03},		//011: 血氧模式（SpO2 Mode）- 使用红色和红外 LED 进行测量（Red and IR）：适用于血氧饱和度检测
	{SPO2_CONFIGURATION, 	0x2e},		//00101010: 01:4096 计数（适用于中等光强度信号）;  010:1000 SPS采样率;  10: 215µs脉冲宽度，对应17位ADC分辨率  
	{LED1_PULSE_AMPLITUDE, 	0x17},		// 设置红色 LED1 的发射电流级别，即发射 LED 的强度, 0x00-0xff 	（原为：0x17）
	{LED2_PULSE_AMPLITUDE, 	0x17},		// 设置红色 LED2 的发射电流级别，即发射 LED 的强度, 0x00-0xff	（原为：0x17）
	{TEMPERATURE_CONFIG, 	0x01},		// 启动温度测量 	(原未配置)
};

/* Private Functions Declare ----------------------------------------------------------*/
static int max30102_fifo_read(void *param, uint32_t *pun_red_led, uint32_t *pun_ir_led);
static void max30102_period_func(void *param);
static int max30102_get_data(const c_max30102* p_obj, uint32_t* heartrate, uint32_t* o2);

void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, int32_t *pn_heart_rate, int8_t *pch_hr_valid);
void maxim_find_peaks(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height);
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size);
void maxim_sort_indices_descend(int32_t  *pn_x, int32_t *pn_indx, int32_t n_size);
/* Private Variables ------------------------------------------------------------------*/
// uch_spo2_table 通过 -45.060 * ratioAverage * ratioAverage + 30.354 * ratioAverage + 94.845 近似计算
const uint8_t uch_spo2_table[184] = { 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
                                      99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                      100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
                                      97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
                                      90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
                                      80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
                                      66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
                                      49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
                                      28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
                                      3, 2, 1};

static  int32_t an_x[ BUFFER_SIZE]; //ir
static  int32_t an_y[ BUFFER_SIZE]; //red
									  
/* Public Functions -------------------------------------------------------------------*/
c_max30102 max30102_create(c_my_iic *iic, uint32_t *red_buf, uint32_t *ir_buf)
{
	c_max30102 new = {0};	//新建一个空的GPIO对象		
	int ret;
    /*参数检查*/
    if(NULL == iic || NULL == iic->this){
        return new;
    }
	/*为 MAX30102 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_max30102));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_max30102));
	
	/*初始化 MAX30102 对象的私有成员*/
	((s_max30102*)new.this)->iic 				= iic;
	((s_max30102*)new.this)->red_buf			= red_buf;
	((s_max30102*)new.this)->ir_buf				= ir_buf;
	
	((s_max30102*)new.this)->hrValidCnt			= 0;
	((s_max30102*)new.this)->hrThrowOutSamp		= 0;
	((s_max30102*)new.this)->hrTimeout			= 0;
		
	((s_max30102*)new.this)->spo2ValidCnt		= 0;
	((s_max30102*)new.this)->spo2ThrowOutSamp 	= 0;
	((s_max30102*)new.this)->spo2Timeout		= 0;
	
	((s_max30102*)new.this)->COUNT				= 0;
	((s_max30102*)new.this)->s_theHeartRate		= 0;
	((s_max30102*)new.this)->s_theO2			= 0;

	
	/*初始化 MAX30102 对象的公有接口*/
	new.get 				= max30102_get_data;

	/*MAX30102 传感器初始化配置*/
	for(uint8_t i = 0; i < sizeof(max_cfg)/sizeof(max30102_cfg_t); i++){
		ret = iic->write_regs(iic, MAX30102_IIC_ADDR, max_cfg[i].reg, &max_cfg[i].val, 1);
		if(ret != R_OK)	goto errorHandle;
	}

	/*读中断状态寄存器, 读完会自动清空，这里其实就是清空中断状态标志*/
	uint8_t data;
	ret = iic->read_regs(iic, MAX30102_IIC_ADDR, INTERRUPT_STATUS1, &data, 1);
	if(ret != R_OK)	goto errorHandle;

	ret = iic->read_regs(iic, MAX30102_IIC_ADDR, INTERRUPT_STATUS2, &data, 1);
	if(ret != R_OK)	goto errorHandle;

	/*注册10ms周期任务函数，注册成功后需要在 while 中调用 my_systick.period_task_running(); 才会执行注册的函数*/
	ret = my_systick.period_task_register(MY_SYSTICK_PERIOD_100MS, max30102_period_func, new.this);
	if(HAL_OK != ret)	goto errorHandle;
	
	
	s_max30102 *this = (s_max30102*)new.this;
	uint32_t un_min, un_max, un_brightness;  // 计算LED亮度的变量，反映心跳
	un_brightness = 0; // 初始化 LED 亮度
	un_min = 0x3FFFF; // 初始化信号最小值
	un_max = 0; // 初始化信号最大值
	int32_t i;
	
	this->n_ir_buffer_length = 150; // 缓冲区长度为 150，存储 3 秒的样本（以 50 次/秒采样）

    // 读取前 150 个样本，并确定信号范围
    for(i = 0; i < this->n_ir_buffer_length; i++)
    {
        // 等待 KEY0 按键被按下
        // while(KEY0 == 1); // 等待中断引脚断言
        max30102_fifo_read(this, (this->red_buf + i), (this->ir_buf + i)); // 从 MAX30102 FIFO 中读取数据

        // 更新信号的最小值和最大值
        if(un_min > this->red_buf[i])
            un_min = this->red_buf[i]; // 更新信号最小值
        if(un_max < this->red_buf[i])
            un_max = this->red_buf[i]; // 更新信号最大值
    }
    this->un_prev_data = this->red_buf[i]; // 保存上一个数据值
    // 在处理完前 150 个样本后（第一个 3 秒样本），计算心率和 SPO2
    maxim_heart_rate_and_oxygen_saturation(this->ir_buf, this->n_ir_buffer_length, this->red_buf, &this->n_spo2, &this->ch_spo2_valid, &this->n_heart_rate, &this->ch_hr_valid);

    // 持续从 MAX30102 获取样本，每秒计算一次心率和 SPO2
	
	return new;

errorHandle:
	new.this = NULL;
	return new;
}

static int max30102_get_data(const c_max30102* p_obj, uint32_t* heartrate, uint32_t* o2)
{
	s_max30102 *this;
    if(p_obj == NULL || heartrate == NULL || o2 == NULL){
        return R_NULL;
    }
	
    this = p_obj->this;
		
	if(this->s_theHeartRate > 190)	*heartrate 	= 0;
	else							*heartrate 	= this->s_theHeartRate;
	
	if(this->s_theO2 > 100)			*o2			= 0;
	else							*o2   		= this->s_theO2;

    return R_OK;
}

/* Private Functions -------------------------------------------------------------------*/
static void max30102_period_func(void *param)
{
	s_max30102 *this = NULL;
	if(param == NULL){
		return;
	}
	this = param;
	
	int32_t i = 0; 				// 重置循环索引
	uint32_t un_min = 0x3FFFF; 	// 重置信号最小值
	uint32_t un_max = 0; 		// 重置信号最大值

	// 将前 50 个样本抛弃，并将最后 100 个样本移动到顶部
	for(i = 50; i < 150; i++){
		this->red_buf[i - 50] = this->red_buf[i]; // 移动红色传感器数据
		this->ir_buf[i - 50] = this->ir_buf[i]; // 移动红外传感器数据

		// 更新信号的最小和最大值
		if(un_min > this->red_buf[i])
				un_min = this->red_buf[i]; // 更新信号最小值
		if(un_max < this->red_buf[i])
				un_max = this->red_buf[i]; // 更新信号最大值
	}

	// 在计算心率之前取 50 个样本
	for(i = 100; i < 150; i++){
		this->un_prev_data = this->red_buf[i - 1]; // 获取之前的样本
		max30102_fifo_read(this, (this->red_buf + i), (this->ir_buf + i)); // 从 MAX30102 FIFO 中读取数据
		// 计算 LED 的亮度
		if(this->red_buf[i] > this->un_prev_data) {					// 如果当前样本大于之前的样本
			this->f_temp = this->red_buf[i] - this->un_prev_data; 	// 计算变化量
			this->f_temp /= (un_max - un_min); 						// 归一化
			this->f_temp *= MAX_BRIGHTNESS; 						// 计算最大亮度
			this->f_temp = this->un_brightness - this->f_temp; 		// 更新亮度
			if(this->f_temp < 0)	this->un_brightness = 0; 		// 防止亮度为负值
			else					this->un_brightness = (int)this->f_temp; // 更新亮度值
		}else {														// 当前样本少于或等于之前的样本
				this->f_temp = this->un_prev_data - this->red_buf[i]; // 计算变化量
				this->f_temp /= (un_max - un_min); 					// 归一化
				this->f_temp *= MAX_BRIGHTNESS; 					// 计算最大亮度
				this->un_brightness += (int)this->f_temp; 			// 更新亮度值
				if(this->un_brightness > MAX_BRIGHTNESS) 			// 最大亮度限制
						this->un_brightness = MAX_BRIGHTNESS; 
		}
	}

	// 计算心率和 SPO2
	maxim_heart_rate_and_oxygen_saturation(this->ir_buf, this->n_ir_buffer_length, this->red_buf, &this->n_spo2, &this->ch_spo2_valid, &this->n_heart_rate, &this->ch_hr_valid);
	
	if(this->COUNT++ > 0){	// 每 5 次采样进行一次处理
		this->COUNT = 0; // 重置计数器
		
		// 检查心率是否有效且在合理范围内
		if ((this->ch_hr_valid == 1) && (this->n_heart_rate < 190) && (this->n_heart_rate > 40)){
			// 重置心率超时
			this->hrTimeout = 0; 
			
			// 如果心率状态有效计数达到 4，则准备丢弃一个样本
			if (this->hrValidCnt == 4){
				this->hrThrowOutSamp = 1; 	// 标记丢弃样本
				this->hrValidCnt = 0; 		// 重置有效计数
				for (i = 12; i < 16; i++){
					// 如果当前心率与最近的样本差异过小，保持样本
					if (this->n_heart_rate < this->hr_buf[i] + 10){
							this->hrThrowOutSamp = 0;
							this->hrValidCnt   = 4; 		// 重新计数有效心率
					}
				}
			}else{
				this->hrValidCnt = this->hrValidCnt + 1; 	// 增加有效心率计数
			}

			 // 如果没有丢弃样本
			if (this->hrThrowOutSamp == 0){
				// 将新样本移入缓冲区
				for(i = 0; i < 15; i++){
						this->hr_buf[i] = this->hr_buf[i + 1]; // 移动样本
				}
				this->hr_buf[15] = this->n_heart_rate; 		// 添加新样本

				// 更新缓冲区填充情况
				if (this->hrBuffFilled < 16){
						this->hrBuffFilled = this->hrBuffFilled + 1; // 增加填充计数
				}

				// 计算移动平均值
				this->hrSum = 0;
				if (this->hrBuffFilled < 2){
					this->hrAvg = 0; 				// 若样本数少于 2，心率平均值为 0 
				}else if (this->hrBuffFilled < 4){
					for(i = 14; i < 16; i++){
							this->hrSum = this->hrSum + this->hr_buf[i]; // 计算总和
					}
					this->hrAvg = this->hrSum >> 1; // 更新平均值
				}else if (this->hrBuffFilled < 8){
					for(i = 12; i < 16; i++){
							this->hrSum = this->hrSum + this->hr_buf[i]; // 计算总和
					}
					this->hrAvg = this->hrSum >> 2; // 更新平均值
				}else if (this->hrBuffFilled < 16){
					for(i = 8; i < 16; i++){
							this->hrSum = this->hrSum + this->hr_buf[i]; // 计算总和
					}
					this->hrAvg = this->hrSum >> 3; // 更新平均值
				}else{
					for(i = 0; i < 16; i++){
						this->hrSum = this->hrSum + this->hr_buf[i]; // 计算总和
					}
					this->hrAvg = this->hrSum >> 4; // 更新平均值
				}
			}
			this->hrThrowOutSamp = 0; 		// 重置丢弃标记
		}else {								// 如果心率无效
			this->hrValidCnt = 0; 			// 重置有效计数
			if (this->hrTimeout == MAX30102_INVAILD_MAX){		// 如果超时计数达到 20
				this->hrAvg = 0; 			// 心率平均值清零
				this->hrBuffFilled = 0; 	// 心率缓冲区填充数清零
			}else{
				this->hrTimeout++; 			// 增加超时计数
			}
		}

		// 检查 SPO2 是否有效且在合理范围内
		if ((this->ch_spo2_valid == 1) && (this->n_spo2 > 59)){
			this->spo2Timeout = 0; // 重置 SPO2 超时
			// 如果 SPO2 状态有效计数达到 4，则准备丢弃一个样本
			if (this->spo2ValidCnt == 4){
				this->spo2ThrowOutSamp = 1; // 标记丢弃样本
				this->spo2ValidCnt = 0; // 重置有效计数
				for (i = 12; i < 16; i++){
					// 如果当前 SPO2 与最近的样本差异过小，保持样本
					if (this->n_spo2 > this->spo2_buf[i] - 10){
							this->spo2ThrowOutSamp = 0; // 不丢弃
							this->spo2ValidCnt   = 4; // 重新计数有效 SPO2
					}
				}
			}else{
				this->spo2ValidCnt = this->spo2ValidCnt + 1; // 增加有效 SPO2 计数
			}

			if (this->spo2ThrowOutSamp == 0) { 		// 如果没有丢弃样本
				// 将新样本移入缓冲区
				for(i = 0; i < 15; i++){
						this->spo2_buf[i] = this->spo2_buf[i + 1]; // 移动样本
				}
				this->spo2_buf[15] = this->n_spo2; // 添加新样本

				// 更新缓冲区填充情况
				if (this->spo2BuffFilled < 16){
						this->spo2BuffFilled = this->spo2BuffFilled + 1; // 增加填充计数
				}

				// 计算移动平均值
				this->spo2Sum = 0;
				if (this->spo2BuffFilled < 2){
					this->spo2Avg = 0; // 若样本数少于 2，SPO2 平均值为 0 
				}else if (this->spo2BuffFilled < 4){
					for(i = 14; i < 16; i++){
						this->spo2Sum = this->spo2Sum + this->spo2_buf[i]; // 计算总和
					}
					this->spo2Avg = this->spo2Sum >> 1; // 更新平均值
				}else if (this->spo2BuffFilled < 8){
					for(i = 12; i < 16; i++){
						this->spo2Sum = this->spo2Sum + this->spo2_buf[i]; // 计算总和
					}
					this->spo2Avg = this->spo2Sum >> 2; // 更新平均值
				}else if (this->spo2BuffFilled < 16){
					for(i = 8; i < 16; i++){
						this->spo2Sum = this->spo2Sum + this->spo2_buf[i]; // 计算总和
					}
					this->spo2Avg = this->spo2Sum >> 3; // 更新平均值
				}else{
					for(i = 0; i < 16; i++){
						this->spo2Sum = this->spo2Sum + this->spo2_buf[i]; // 计算总和
					}
					this->spo2Avg = this->spo2Sum >> 4; // 更新平均值
				}
			}
			this->spo2ThrowOutSamp = 0; 		// 重置丢弃标记
		}else {									// SPO2 无效
			this->spo2ValidCnt = 0; 			// 重置有效计数
			if (this->spo2Timeout == MAX30102_INVAILD_MAX){		// 如果超时计数达到 4
				this->spo2Avg = 0; 				// SPO2 平均值清零
				this->spo2BuffFilled = 0; 		// SPO2 缓冲区填充数清零
			}else{
				this->spo2Timeout++; 			// 增加超时计数
			}
		}
	}

	// 更新全局心率变量
	if(this->hrAvg >= 0){
		this->s_theHeartRate 	= this->hrAvg; // 更新全局心率
	}
	// 更新全局 SPO2 变量
	if(this->spo2Avg >= 0){
		this->s_theO2 			= this->spo2Avg; // 更新全局 SPO2
	}
	
//	// 更新全局 SPO2 变量
//	if(0 != this->s_theHeartRate){
//		if(0 < this->spo2Avg){
//			this->s_theO2 = this->spo2Avg; // 更新全局 SPO2
//		}
//	}else{
//		this->s_theO2 = 0; // 如果心率为 0，SPO2 置为 0
//	}
}

static int max30102_fifo_read(void *param, uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
	int ret;
	s_max30102 *this = NULL;
	
	/*检查参数*/
	if(param == NULL || pun_red_led == NULL || pun_ir_led == NULL){
		return R_ERROR;
	}
	this = param;

	*pun_ir_led = 0;
    *pun_red_led = 0;
		
	/*读中断状态寄存器, 读完会自动清空，这里其实就是清空中断状态标志*/
	uint8_t data;
	ret = this->iic->read_regs(this->iic, MAX30102_IIC_ADDR, INTERRUPT_STATUS1, &data, 1);
	if(ret != R_OK)	return R_ERROR;

	ret = this->iic->read_regs(this->iic, MAX30102_IIC_ADDR, INTERRUPT_STATUS2, &data, 1);
	if(ret != R_OK)	return R_ERROR;
	
	uint8_t receive_data[6];
	ret = this->iic->read_regs(this->iic, MAX30102_IIC_ADDR, FIFO_DATA, receive_data, 6);
	if(ret != R_OK)		return R_ERROR;
	
    *pun_red_led = ((receive_data[0]<<16 | receive_data[1]<<8 | receive_data[2]) & 0x03ffff);
    *pun_ir_led = ((receive_data[3]<<16 | receive_data[4]<<8 | receive_data[5]) & 0x03ffff);

	return R_OK;
}




/* 算法函数  ----------------------------------------------------------------------*/

/**
* \brief        Calculate the heart rate and SpO2 level
* \par          Details
*               By detecting  peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the an_ratio for the SPO2 is computed.
*               Since this algorithm is aiming for Arm M0/M3. formaula for SPO2 did not achieve the accuracy due to register overflow.
*               Thus, accurate SPO2 is precalculated and save longo uch_spo2_table[] per each an_ratio.
*
* \param[in]    *pun_ir_buffer           - IR sensor data buffer
* \param[in]    n_ir_buffer_length      - IR sensor data buffer length
* \param[in]    *pun_red_buffer          - Red sensor data buffer
* \param[out]    *pn_spo2                - Calculated SpO2 value
* \param[out]    *pch_spo2_valid         - 1 if the calculated SpO2 value is valid
* \param[out]    *pn_heart_rate          - Calculated heart rate value
* \param[out]    *pch_hr_valid           - 1 if the calculated heart rate value is valid
*
* \retval       None
*/
void maxim_heart_rate_and_oxygen_saturation(uint32_t *	pun_ir_buffer, 
											int32_t 	n_ir_buffer_length, 
											uint32_t *	pun_red_buffer, 
											int32_t *	pn_spo2, 
											int8_t *	pch_spo2_valid,
											int32_t *	pn_heart_rate, 
											int8_t *	pch_hr_valid)
{
    uint32_t un_ir_mean, un_only_once ;
    int32_t k, n_i_ratio_count;
    int32_t i, s, m, n_exact_ir_valley_locs_count, n_middle_idx;
    int32_t n_th1, n_npks, n_c_min;
    int32_t an_ir_valley_locs[15] ;
    int32_t n_peak_interval_sum;

    int32_t n_y_ac, n_x_ac;
    int32_t n_spo2_calc;
    int32_t n_y_dc_max, n_x_dc_max;
    int32_t n_y_dc_max_idx, n_x_dc_max_idx;
    int32_t an_ratio[5], n_ratio_average;
    int32_t n_nume, n_denom ;

    // calculates DC mean and subtract DC from ir
    un_ir_mean = 0;
    for (k = 0 ; k < n_ir_buffer_length ; k++ ) un_ir_mean += pun_ir_buffer[k] ;
    un_ir_mean = un_ir_mean / n_ir_buffer_length ;

    // remove DC and invert signal so that we can use peak detector as valley detector
    for (k = 0 ; k < n_ir_buffer_length ; k++ )
        an_x[k] = -1 * (pun_ir_buffer[k] - un_ir_mean) ;

    // 4 pt Moving Average
    for(k = 0; k < BUFFER_SIZE - MA4_SIZE; k++)
    {
        an_x[k] = ( an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]) / (int)4;
    }
    // calculate threshold
    n_th1 = 0;
    for ( k = 0 ; k < BUFFER_SIZE ; k++)
    {
        n_th1 +=  an_x[k];
    }
    n_th1 =  n_th1 / ( BUFFER_SIZE);
    if( n_th1 < 30) n_th1 = 30; // min allowed
    if( n_th1 > 60) n_th1 = 60; // max allowed

    for ( k = 0 ; k < 15; k++) an_ir_valley_locs[k] = 0;
    // since we flipped signal, we use peak detector as vSalley detector
    maxim_find_peaks( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, 15 );//peak_height, peak_distance, max_num_peaks
    n_peak_interval_sum = 0;
    if (n_npks >= 2)
    {
        for (k = 1; k < n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] - an_ir_valley_locs[k - 1] ) ;
        n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1);
        *pn_heart_rate = (int32_t)( (FS * 60) / n_peak_interval_sum );
        *pch_hr_valid  = 1;
    }
    else
    {
        *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
        *pch_hr_valid  = 0;
    }

    //  load raw value again for SPO2 calculation : RED(=y) and IR(=X)
    for (k = 0 ; k < n_ir_buffer_length ; k++ )
    {
        an_x[k] =  pun_ir_buffer[k] ;
        an_y[k] =  pun_red_buffer[k] ;
    }

    // find precise min near an_ir_valley_locs
    n_exact_ir_valley_locs_count = n_npks;

    //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
    //finding AC/DC maximum of raw

    n_ratio_average = 0;
    n_i_ratio_count = 0;
    for(k = 0; k < 5; k++) an_ratio[k] = 0;
    for (k = 0; k < n_exact_ir_valley_locs_count; k++)
    {
        if (an_ir_valley_locs[k] > BUFFER_SIZE )
        {
            *pn_spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
            *pch_spo2_valid  = 0;
            return;
        }
    }
    // find max between two valley locations
    // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2
    for (k = 0; k < n_exact_ir_valley_locs_count - 1; k++)
    {
        n_y_dc_max = -16777216 ;
        n_x_dc_max = -16777216;
        if (an_ir_valley_locs[k + 1] - an_ir_valley_locs[k] > 3)
        {
            for (i = an_ir_valley_locs[k]; i < an_ir_valley_locs[k + 1]; i++)
            {
                if (an_x[i] > n_x_dc_max)
                {
                    n_x_dc_max = an_x[i];
                    n_x_dc_max_idx = i;
                }
                if (an_y[i] > n_y_dc_max)
                {
                    n_y_dc_max = an_y[i];
                    n_y_dc_max_idx = i;
                }
            }
            n_y_ac = (an_y[an_ir_valley_locs[k + 1]] - an_y[an_ir_valley_locs[k] ] ) * (n_y_dc_max_idx - an_ir_valley_locs[k]); //red
            n_y_ac =  an_y[an_ir_valley_locs[k]] + n_y_ac / (an_ir_valley_locs[k + 1] - an_ir_valley_locs[k])  ;
            n_y_ac =  an_y[n_y_dc_max_idx] - n_y_ac;   // subracting linear DC compoenents from raw
            n_x_ac = (an_x[an_ir_valley_locs[k + 1]] - an_x[an_ir_valley_locs[k] ] ) * (n_x_dc_max_idx - an_ir_valley_locs[k]); // ir
            n_x_ac =  an_x[an_ir_valley_locs[k]] + n_x_ac / (an_ir_valley_locs[k + 1] - an_ir_valley_locs[k]);
            n_x_ac =  an_x[n_y_dc_max_idx] - n_x_ac;     // subracting linear DC compoenents from raw
            n_nume = ( n_y_ac * n_x_dc_max) >> 7 ; //prepare X100 to preserve floating value
            n_denom = ( n_x_ac * n_y_dc_max) >> 7;
            if (n_denom > 0  && n_i_ratio_count < 5 &&  n_nume != 0)
            {
                an_ratio[n_i_ratio_count] = (n_nume * 100) / n_denom ; //formular is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max) ;
                n_i_ratio_count++;
            }
        }
    }
    // choose median value since PPG signal may varies from beat to beat
    maxim_sort_ascend(an_ratio, n_i_ratio_count);
    n_middle_idx = n_i_ratio_count / 2;

    if (n_middle_idx > 1)
        n_ratio_average = ( an_ratio[n_middle_idx - 1] + an_ratio[n_middle_idx]) / 2; // use median
    else
        n_ratio_average = an_ratio[n_middle_idx ];

    if( n_ratio_average > 2 && n_ratio_average < 184)
    {
        n_spo2_calc = uch_spo2_table[n_ratio_average] ;
        *pn_spo2 = n_spo2_calc ;
        *pch_spo2_valid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
    }
    else
    {
        *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
        *pch_spo2_valid  = 0;
    }
}

/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
* \retval       None
*/
void maxim_find_peaks( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
{
    maxim_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
    maxim_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
    *n_npks = min( *n_npks, n_max_num );
}

/**
* \brief        Find peaks above n_min_height
* \par          Details
*               Find all peaks above MIN_HEIGHT
* \retval       None
*/
void maxim_peaks_above_min_height( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height )
{
    int32_t i = 1, n_width, riseFound = 0, holdOff1 = 0, holdOff2 = 0, holdOffThresh = 4;
    *n_npks = 0;

    while (i < n_size - 1)
    {
        if (holdOff2 == 0)
        {
            if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i - 1])     // find left edge of potential peaks
            {
                riseFound = 1;
            }
            if (riseFound == 1)
            {
                if ((pn_x[i] < n_min_height) && (holdOff1 < holdOffThresh))     // if false edge
                {
                    riseFound = 0;
                    holdOff1 = 0;
                }
                else
                {
                    if (holdOff1 == holdOffThresh)
                    {
                        if ((pn_x[i] < n_min_height) && (pn_x[i - 1] >= n_min_height))
                        {
                            if ((*n_npks) < 15 )
                            {
                                pn_locs[(*n_npks)++] = i;   // peak is right edge
                            }
                            holdOff1 = 0;
                            riseFound = 0;
                            holdOff2 = 8;
                        }
                    }
                    else
                    {
                        holdOff1 = holdOff1 + 1;
                    }
                }
            }
        }
        else
        {
            holdOff2 = holdOff2 - 1;
        }
        i++;
    }
}

/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
* \retval       None
*/
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
{

    int32_t i, j, n_old_npks, n_dist;

    /* Order peaks from large to small */
    maxim_sort_indices_descend( pn_x, pn_locs, *pn_npks );

    for ( i = -1; i < *pn_npks; i++ )
    {
        n_old_npks = *pn_npks;
        *pn_npks = i + 1;
        for ( j = i + 1; j < n_old_npks; j++ )
        {
            n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
            if ( n_dist > n_min_distance || n_dist < -n_min_distance )
                pn_locs[(*pn_npks)++] = pn_locs[j];
        }
    }

    // Resort indices int32_to ascending order
    maxim_sort_ascend( pn_locs, *pn_npks );
}

/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
* \retval       None
*/
void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size)
{
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++)
    {
        n_temp = pn_x[i];
        for (j = i; j > 0 && n_temp < pn_x[j - 1]; j--)
            pn_x[j] = pn_x[j - 1];
        pn_x[j] = n_temp;
    }
}

/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
* \retval       None
*/
void maxim_sort_indices_descend(  int32_t  *pn_x, int32_t *pn_indx, int32_t n_size)
{
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++)
    {
        n_temp = pn_indx[i];
        for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j - 1]]; j--)
            pn_indx[j] = pn_indx[j - 1];
        pn_indx[j] = n_temp;
    }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/********************************************************************************
DSP库移植：https://blog.51cto.com/u_16213663/11402548
DSP库测试：注意工程勾选 Use MicroLIB
包含以下头文件：
#include "arm_math.h"
#include "arm_const_structs.h"
测试代码：
float test = arm_sin_f32(3.1415926/6)+1;
测试结果：test = 1.49999

2.引脚介绍：
GND：接地线;
RD:MAX30102芯片的RED,LED接地端，一般不接（驱动红色LED）
IRD: MAX30102芯片的RLED接地端，一般不接（驱动红外LED）
INT:MAX30102芯片的中断引脚
VIN:主电源电源输入端， 1.8V-5V;3位焊盘:选择总线的上拉电平,取决于引脚主控电压,可选1.8V端或者3.3V端（此端包含3.3V及以上）；
SCL：接I2C总线的时钟；
SDA：接12C总线的数据；

参考资料：《野火小智心率血氧检测模块规格手册_20240425》

1. MAX30102 心率血氧模块简介:
	心率血氧检测模块是一种集成了 红外光和可见红光 LED、光电探测器 以及信号处理电路的设备， 利
	用人体组织在血管搏动时造成透光率不同来实时监测心率和血氧饱和度 ，主芯片为 MAX30102，使用 IIC接口通信
注：测量时请尽量保持模块和手指相对静止，模块不移动和抖动

2. 参数特性
◆ 超低功耗：< 1mW
◆ 超低待机电流 ：0.7μA
◆ 内部采样 ADC：15~18 位，受 LED 脉冲宽度配置影响
◆ 工作电压范围：3.3V~5V
◆ 工作温度范围：-40℃~ +85℃

3.检测原理：
 -心率血氧模块对外发射可见红光（RED）和红外光（IR）
 -光电探测器（VISIBLE+IR）会检测通过皮肤反射回来的光线并将其转换为电信号送入模块内部的信号处理电路，
 -电路对这些反射光进行放大、环境光消除、模数转换和滤波处理，处理后将数据保存在 FIFO 数据寄存器里，主控通过 I2C 接口读取数据寄存器数据
 -红光和红外光穿透人体组织，并使用光电探测器测量反射光量，也就是利用人体组织在血管搏动时造成透光率不同来进行心率和血氧饱和度测量
 -血液中有 含氧血红蛋白（O2Hb）和 脱氧血红蛋白（HHb），这两种血红蛋白的吸收光谱不一样，
 -脱氧血红蛋白（HHb）吸收更多的红光，而含氧血红蛋白（O2Hb）吸收更多的红外光

4.心率测量：
当心脏跳动时，血液被泵入和泵出，反射回来的光线强度会发生变化（吸收红外光 IR 的量会变化），
光电探测器接收透过皮肤反射回来的光线，将其转化为电信号并产生变化的波形，通过测量这些电信号的变化来计算出心率

5.血氧测量：
通过光电探测器检测反射回来的光线强度，可以判断红光、红外光被吸收的多少，
从而推断出含氧血红蛋白（O2Hb）和脱氧血红蛋白（HHb）所占的比例可以估算血液中的氧气含量，并计算出血氧饱和度

6.程序流程:
 -MAX30102 芯片的 FIFO 数据寄存器可存储 32 个数据样本，每个样本大小取决于配置通道的数量
 -当配置为 Heart Rate 模式时，每个样本就是一个 IR 通道组成，3 个字节数据大小
 -当配置为 SpO2 模式时，每个样本由 RED 和 IR 两个通道组成，6 个字节的数据大小，前 3 个字节为RED 数据，后 3 个字节为 IR 数据，因此 FIFO 最多可以存储 192 个字节的数据
 -读取 FIFO_DATA 寄存器时，不会自动递增 I2C 寄存器地址，而是从同一个地址反复读取数据，
 -在Heart Rate 模式下，一个样本为 3 个字节数据，需要调用 3 次 I2C 字节读取才能获得一个完整的样本

 7.寄存器定义:
INTERRUPT_STATUS1:
	Bit 7: 表示 FIFO 几乎已满。当 FIFO 中的数据量接近满时触发此中断。
	Bit 6: 表示 PPG （电容积脉光搏波）数据准备好，可以从 FIFO 中读取。
	Bit 5: 表示环境光消除模（ALC）块溢出。当环境光过强导致传感器无法正确采集数据时触发。
	Bit 4-1: Reserved 保留位，未使用，通常应设置为 0。
	Bit 0: 当设备启动完成并准备好工作时，该位被置 1。此标志位通常在设备上电初始化时使用。

INTERRUPT_STATUS2:
	Bit 7-2: Reserved
	Bit 1: 表示芯片内温度传感器的温度数据已准备好，可以读取。这个位在温度数据准备好时会被设置为 1。
	Bit 0: Reserved

FIFO_CONFIGURATION：
	Bits 7:5: 这些位用于设置每个通道的采样平均数。通过对相邻的采样点进行平均，可以减少传输的数据量，并可能减少噪声。
	000: 平均 1 个样本（无平均）
	001: 平均 2 个样本
	010: 平均 4 个样本
	011: 平均 8 个样本
	100: 平均 16 个样本
	101: 平均 32 个样本
	110: 平均 32 个样本
	111: 平均 32 个样本

Bit 4:控制当 FIFO 填满数据时的行为
	1 - 启用回滚模式。当 FIFO 填满时，写指针会回滚到 0，继续填充新的数据。这意味着旧的数据会被覆盖。
	0 - 禁用回滚模式。当 FIFO 填满时，不再写入新数据，直到读取一些数据或修改写/读指针位置为止。

Bits 3:0: 设定当 FIFO 中剩余多少个数据样本时触发中断。
	每个数据样本为 3 字节。这个字段的值表示在触发中断时，FIFO 中还剩下多少个数据样本未读。例如：
	0x0: 当 FIFO 满（32 个样本全满）时触发中断。
	0xF: 当 FIFO 还剩下 15 个样本（已写入 17 个样本）时触发中断。

MODE_CONFIGURATION：
	Bit 7: 控制设备进入省电模式。
	值: 1 - 启用关机模式（Shutdown）。设备进入省电模式，所有内部操作暂停，但所有寄存器保留其值，读写操作仍然有效。在这种模式下，所有中断会被清除为零。
	值: 0 - 退出关机模式，设备恢复正常操作。

	Bit 6: 控制设备复位。
	值: 1 - 执行复位操作。所有配置寄存器、阈值寄存器和数据寄存器都会被重置为上电状态的默认值。复位完成后，RESET 位会自动清零。
	注意: 设置 RESET 位不会触发 PWR_RDY 中断事件。复位过程中，所有之前的配置将丢失。

	Bits 2:0: 设置 MAX30102 的工作模式。
	010 (0x2): 心率模式（Heart Rate Mode）- 仅使用红色 LED 进行测量（Red only）：适用于仅测量心率的应用
	011 (0x3): 血氧模式（SpO2 Mode）- 使用红色和红外 LED 进行测量（Red and IR）：适用于血氧饱和度检测
	111 (0x7): 多 LED 模式（Multi-LED Mode）- 使用多个 LED 进行测量（Red and IR）：适用于需要同时使用多种 LED 进行检测的应用

SPO2_CONFIGURATION:
	Bits 6:5: 设置 SpO2 传感器 ADC 的全尺度范围。
	00: 2048 计数（适用于高光强度信号）
	01: 4096 计数（适用于中等光强度信号）
	10: 8192 计数（适用于低光强度信号）
	11: 16384 计数（适用于非常低光强度信号）
	这个设置允许传感器在不同的光强度条件下进行最佳化，使得传感器可以适应不同强度的信号。

	Bits 4:2: 定义有效的采样率，每个样本包括一个红色 LED 脉冲/转换和一个红外 LED 脉冲/转换。
	000: 250 SPS (样本每秒)
	001: 500 SPS
	010: 1000 SPS
	011: 2000 SPS
	100: 4000 SPS
	101-111: 保留或不使用
	注意: 采样率与脉冲宽度相关。选择的采样率必须与脉冲宽度设置相匹配。如果选择的采样率超过了所选脉冲宽度的最大允许值，寄存器会自动编程为最高可能的采样率。

	Bits 1:0: 设置  LED 发射脉冲的宽度(影响每个样本的采集时间) 和 ADC 分辨率。IR 和红色 LED 的脉冲宽度设置相同。
	00: 69 µs 脉冲宽度，对应 15 位 ADC 分辨率
	01: 118 µs 脉冲宽度，对应 16 位 ADC 分辨率
	10: 215 µs 脉冲宽度，对应 17 位 ADC 分辨率
	11: 411 µs 脉冲宽度，对应 18 位 ADC 分辨率

MULTILED1_MODE / MULTILED2_MODE: (Multi-LED Mode下需要用到，一般无需关注)
	在 Multi-LED Mode 下，MAX30102 允许在每个样本中使用多达四个时间槽（SLOT1 到 SLOT4），
	并通过控制寄存器 (0x11 和 0x12) 配置每个时间槽中的 LED 活动状态。这种配置提供了极大的灵活性，可以根据需要选择不同的 LED 来进行测量。

	寄存器 0x11:  配置在 SLOT1 和 SLOT2 中的 LED 活动状态。
	0x00: SLOT1 不使用，SLOT2 不使用
	0x01: SLOT1 使用红色 LED (LED1)，SLOT2 不使用
	0x02: SLOT1 不使用，SLOT2 使用红外 LED (LED2)
	0x03: SLOT1 使用红色 LED (LED1)，SLOT2 使用红外 LED (LED2)
	0x04: SLOT1 使用绿色 LED，SLOT2 不使用 (如果支持绿色 LED)
	0x05: SLOT1 使用绿色 LED，SLOT2 使用红色 LED (LED1)
	0x06: SLOT1 使用绿色 LED，SLOT2 使用红外 LED (LED2)
	0x07: SLOT1 和 SLOT2 分别使用绿色、红色和红外 LED

	寄存器 0x12:  配置在 SLOT3 和 SLOT4 中的 LED 活动状态。
	0x00: SLOT3 不使用，SLOT4 不使用
	0x01: SLOT3 使用红色 LED (LED1)，SLOT4 不使用
	0x02: SLOT3 不使用，SLOT4 使用红外 LED (LED2)
	0x03: SLOT3 使用红色 LED (LED1)，SLOT4 使用红外 LED (LED2)
	0x04: SLOT3 使用绿色 LED，SLOT4 不使用 (如果支持绿色 LED)
	0x05: SLOT3 使用绿色 LED，SLOT4 使用红色 LED (LED1)
	0x06: SLOT3 使用绿色 LED，SLOT4 使用红外 LED (LED2)
	0x07: SLOT3 和 SLOT4 分别使用绿色、红色和红外 LED


*/