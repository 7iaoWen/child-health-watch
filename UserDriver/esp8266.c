/********************************************************************************
* @file		:esp8266.c
* @brief	:esp8266 短信模块驱动程序（改为异步的，需要 在main中调用，my_systick.period_task_running();）
* @Author	:Gwen9
* @Date		:2024/08/12
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_ESP8266_ENABLED

/* Private Macros ----------------------------------------------------------*/
#define ESP8266_BAUDRATE            (115200)	//定义ESP8266串口通信波特率
#define ESP8266_SEND_MAX_LEN		(256)		//ESP8266 发送接收最大数据缓存长度
#define ESP8266_REC_MAX_LEN			(256)		//ESP8266 发送接收最大数据缓存长度

/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _S_ESP8266
{
	my_uart_id_t 		uart_id;
	work_mode_t			mode;				//工作模式
    esp_sta_t			state;				//设备状态: 未初始化，繁忙，空闲
	esp_connect_sta_t	connect;			//网络连接状态

	uint8_t				retry_time;			//重发次数
	uint8_t				step;				//状态机状态
	uint8_t				ack_state;			//响应状态
	
	char	*			rec_buf;			//指向接收数据缓存
	char	*			send_buf;			//指向发送数据缓存
	uint16_t			rec_cnt;			//接收技术计数

    char       *		expt_rec;    		//期望接收数组

    const char *      	mqtt_key;			//产品ID
    const char *		mqtt_dev_name;		//设备名称
    const char *       mqtt_dev_secret;	//密钥

	uint8_t 			timer_id;  			/*软件定时器ID*/
	uint32_t			ack_overtime;		/*响应超时时间*/
	uint32_t 			wait_overtime;		/*等待超时时间*/
	int                (*recive_callback)(void* param, char* key, char* value);  /*mqtt接收回调函数*/
	void				*param;
	void			   (*show_info)(char *);											  /*信息打印函数*/
}s_esp8266;

/* Private Functions Declare ----------------------------------------------------------*/
static int esp8266_at_cmd(s_esp8266* this, const char *cmd, char *expt, uint32_t time_out);
static void esp8266_uart_callback(void* param, uint8_t* data, uint16_t data_len);
static void my_esp8266_period_func(void *p_s_esp8266);
static uint8_t esp8266_extract_params(const char* input, char* key, char* value);

static int esp8266_get_state(const c_esp8266* p_obj);
static int esp8266_send(const c_esp8266* p_obj, const char *data);
static int esp8266_set_recive_callback(const c_esp8266* p_obj, int(*callback)(void* param, char* key, char* value), void *param);

/* Private Variables ------------------------------------------------------------------*/
static char rec_buf[ESP8266_REC_MAX_LEN];
static char send_buf[ESP8266_SEND_MAX_LEN];
static char key[32];
static char value[32];
/* Public Functions -------------------------------------------------------------------*/
c_esp8266 esp8266_creat(my_uart_id_t uart_id, work_mode_t mode, const char *key, const char *dev_name, const char *dev_secret)
{
	c_esp8266 new = {0};	//新建一个空的GPIO对象		
	int ret;

	/*参数检测*/
	if(uart_id >= UART_MAX){
		return new;
	}
	
	/*为 ESP8266 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_esp8266));
	if(NULL == new.this){
		return new;
	}
	memset(new.this, 0, sizeof(s_esp8266));

	/*初始化 ESP8266 对象的私有成员*/
	((s_esp8266*)new.this)->uart_id 		= uart_id;
	((s_esp8266*)new.this)->mode			= mode;		
	((s_esp8266*)new.this)->rec_buf			= rec_buf;
	((s_esp8266*)new.this)->send_buf		= send_buf;
	((s_esp8266*)new.this)->expt_rec		= NULL;
	((s_esp8266*)new.this)->state   		= ESP8266_UNINITED;
	((s_esp8266*)new.this)->mqtt_key		= key;
	((s_esp8266*)new.this)->mqtt_dev_name 	= dev_name;
	((s_esp8266*)new.this)->mqtt_dev_secret = dev_secret;
	((s_esp8266*)new.this)->ack_overtime	= 0;
	((s_esp8266*)new.this)->wait_overtime	= 0;
	((s_esp8266*)new.this)->step			= 0;
	((s_esp8266*)new.this)->connect			= ESP8266_DISCONNECT;
	((s_esp8266*)new.this)->retry_time		= 0;
	((s_esp8266*)new.this)->ack_state		= 0xff; 
	/*注册100ms周期任务函数，注册成功后需要在 while 中调用 my_systick.period_task_running(); 才会执行注册的函数*/
	ret = my_systick.period_task_register(MY_SYSTICK_PERIOD_100MS, my_esp8266_period_func, new.this);
	if(HAL_OK != ret)	goto errorHandle;

	/*初始化 ESP8266 对象的公有接口*/
	new.state 					= esp8266_get_state;
	new.send 					= esp8266_send;
	new.set_recive_callback		= esp8266_set_recive_callback;
	
	/*初始化 ESP8266 用到的串口*/
	ret = my_uart.init(uart_id, ESP8266_BAUDRATE, UART_MODE_DMA);
	if(ret != R_OK)		goto errorHandle;
	
	/*注册 esp8266 的串口回调函数*/
	ret = my_uart.register_callback(uart_id, esp8266_uart_callback, new.this);
	if(ret != R_OK)		goto errorHandle;

	((s_esp8266*)new.this)->state   		= ESP8266_IDEL;

	/*打印8266工作模式*/
	switch(((s_esp8266*)new.this)->mode){
		case AP_TCP				:	my_log.show("AP TCP...", ESP8266_LOG_SHOW_TIME); 				break;
		case STA_TCP			:	my_log.show("STA TCP...", ESP8266_LOG_SHOW_TIME); 				break;
		case STA_MQTT_ALICLOUD	:	my_log.show("STA MQTT ALI CLOUD...", ESP8266_LOG_SHOW_TIME);	break;
		case STA_MQTT_ONENET	:	my_log.show("STA MQTT ONENET...", ESP8266_LOG_SHOW_TIME); 		break;
		default					: 	my_log.show("MODE ERROR!", ESP8266_LOG_SHOW_TIME);  			break;
	}
	return new;

errorHandle:
	new.this = NULL;
	return new;
}

static int esp8266_get_state(const c_esp8266* p_obj)
{
	s_esp8266 *this = NULL;

    /*检查参数*/
    if(NULL == p_obj){
        return R_NULL;
    }
	this = p_obj->this;
	
	return this->connect;
}

static int esp8266_send(const c_esp8266* p_obj, const char *data)
{
	s_esp8266 *this = NULL;
	int ret;
	
    /*检查参数*/
    if(NULL == p_obj || data == NULL){
        return R_NULL;
    }
	this = p_obj->this;
	
	/*串口发送数据*/
	memset(this->send_buf, 0, ESP8266_SEND_MAX_LEN);
	
	if(this->mode == STA_MQTT_ALICLOUD){		//阿里云
		snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ALICLOUD_MQTT_PUB, this->mqtt_key, this->mqtt_dev_name, data);		
		my_uart.send(this->uart_id, (uint8_t*)this->send_buf, strlen(this->send_buf));
	}else if(this->mode == STA_MQTT_ONENET){	//ONENET
		snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ONENET_MQTT_PUB, this->mqtt_key, this->mqtt_dev_name, data);		
		my_uart.send(this->uart_id, (uint8_t*)this->send_buf, strlen(this->send_buf));
	}else if(this->mode == AP_TCP){
		/*设置发送内容*/
		uint32_t tx_len = strlen(data);
		snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_TCP_SEND, tx_len);
		my_uart.send(this->uart_id, (uint8_t*)this->send_buf, strlen(this->send_buf));		/*发送命令发送数据*/
		
		/*等待期望接收数据到来*/
		uint8_t i = 0;
		uint32_t rec_len = 0;
		memset(this->rec_buf, 0, ESP8266_REC_MAX_LEN);	//清空接收缓存
		this->rec_cnt = 0;								//复位写指针
		for(uint8_t i = 0; i < 50; i++){
			DelayMs(10);
			ret =  my_uart.recive(this->uart_id, this->rec_buf, &rec_len);
			if(rec_len > 0 && strstr(this->rec_buf, "<") != NULL)	break; 
		}
		if(i < 50){
			my_uart.send(this->uart_id, (uint8_t*)data, strlen(data));						/*发送真实数据*/
		}
	}
	return R_OK;
}

static int esp8266_set_recive_callback(const c_esp8266* p_obj, int(*callback)(void* param,char* key, char* value), void *param)
{
	s_esp8266 *this = NULL;
    /*检查参数*/
    if(NULL == p_obj || callback == NULL){
        return R_NULL;
    }
	this = p_obj->this;

	this->recive_callback = callback;
	this->param			  = param;
	
	return R_OK;
}

/* Private Functions -------------------------------------------------------------------*/
//提取params 后的josn键值对
static uint8_t esp8266_extract_params(const char* input, char* key, char* value) 
{
    const char* paramsStart = strstr(input, "\"params\":{");
    if (paramsStart == NULL) {
        return 1;
    }
    paramsStart += 10; // Move pointer to start of params

    const char* keyStart = strchr(paramsStart, '"') + 1;
    const char* keyEnd = strchr(keyStart, '"');
    const char* valueStart = strstr(keyEnd, ":") + 1;
    const char* valueEnd = strchr(valueStart, '}');

	if(keyStart == NULL || keyEnd == NULL || valueStart == NULL || valueEnd == NULL){
		my_log.show("Recived mqtt data error,can ont find key-value pair !", ESP8266_LOG_SHOW_TIME);
		return 1;
	}
	
    strncpy(key, keyStart, keyEnd - keyStart);
    key[keyEnd - keyStart] = '\0';
    strncpy(value, valueStart, valueEnd - valueStart);
    value[valueEnd - valueStart] = '\0';
	return 0;
}

static void esp8266_uart_callback(void* param, uint8_t* data, uint16_t data_len) 
{ 
    /*检查参数*/
    if(NULL == param || NULL == data || data_len < 6) {
        return ;
    }
	s_esp8266* this = param;
	
 	if(data_len + this->rec_cnt < ESP8266_REC_MAX_LEN){
		memcpy(this->rec_buf + this->rec_cnt, data, data_len);
		this->rec_cnt += data_len;
	}else{
		goto reset_recive;
	}	

	/*收到命令响应处理*/
	if(this->state == ESP8266_BUSY && HAL_GetTick() < this->ack_overtime){
		if(strstr(this->rec_buf, this->expt_rec) != NULL){
			this->state = ESP8266_IDEL;
			this->ack_overtime = 0;
			this->step++;
			goto reset_recive;
		}
	}else if(this->state == ESP8266_IDEL){	/*空闲时数据接收处理*/
		if(strstr(this->rec_buf, "+STA_CONNECTED:") != NULL){			//AP 模式下 STA已连接WIFI
			my_log.show("WIFI CONNECTED", ESP8266_LOG_SHOW_TIME);
			this->connect = ESP8266_WIFI_CONNECTED;
			goto reset_recive;
		}else if(strstr(this->rec_buf, "+STA_DISCONNECTED:") != NULL){	//AP 模式下 STA已断开WIFI
			my_log.show("WIFI DISCONNECT", ESP8266_LOG_SHOW_TIME);
			this->connect = ESP8266_DISCONNECT;	
			this->step = 0;		/*重新连接*/					
			goto reset_recive;
		}else if(strstr(this->rec_buf, "CONNECT\r\n") != NULL){		//TCP 服务器 模式下 客户端已连接
			my_log.show("TCP CONNECT", ESP8266_LOG_SHOW_TIME);	
			this->connect	= ESP8266_TCP_CONNECTED;
			goto reset_recive;
		}else if(strstr(this->rec_buf, "CLOSED\r\n") != NULL){			//TCP 服务器 模式下 客户端已断开
			my_log.show("TCP DISCONNECT", ESP8266_LOG_SHOW_TIME);
			this->connect = ESP8266_DISCONNECT;
			goto reset_recive;
		}else if(strstr(this->rec_buf, "SEND OK\r\n") != NULL){
			goto reset_recive;
		}else if(strstr(this->rec_buf, "+MQTTSUBRECV") != NULL){
			 if(esp8266_extract_params(rec_buf, key, value) == 0){
				if(this->recive_callback != NULL)		this->recive_callback(this, key, value);
				else									my_log.show("MQTT RECVICED DATA! NO CALLBACK", ESP8266_LOG_SHOW_TIME);
				memset(value, 0, sizeof(value));
				memset(key, 0, sizeof(key));
			 }
			goto reset_recive;
		}else if(strstr(this->rec_buf, "+IPD") != NULL){
			 char * index = strstr(rec_buf,"+IPD");
			 index = strstr(index,":");
			 if(index != NULL){
				 char * data_end = strstr(index, "\r\n"); 
				 if(data_end != NULL){
					*data_end = 0;
					if(this->recive_callback != NULL)		this->recive_callback(this, NULL, index+1);
					else									my_log.show("TCP RECVICED DATA! NO CALLBACK", ESP8266_LOG_SHOW_TIME);
					goto reset_recive;
				 }	
			 }
		}
	}
    return ;
	
reset_recive:
	this->rec_cnt = 0;
	memset(this->rec_buf, 0, ESP8266_REC_MAX_LEN);
	return ;
}

static void my_esp8266_period_func(void *p_s_esp8266)
{
	int ret;
	
	/*检查参数*/
    if(NULL == p_s_esp8266) {
        return ;
    }
	s_esp8266* this = p_s_esp8266;

	/*检查参数*/
	if(this->state == ESP8266_UNINITED){
		return;
	}

	/*检查 esp8266 状态*/
	if(this->state == ESP8266_BUSY){				//繁忙状态，等待命令响应
		if(HAL_GetTick() >= this->ack_overtime){	//命令响应超时
			this->state = ESP8266_IDEL;
			this->ack_overtime = 0;
			this->retry_time++;
			this->ack_state = 0xff;					//重置命令响应状态为：无命令
			if(this->retry_time > ESP8266_MAX_RETRY_TIMES){
				this->retry_time = 0;
				this->step = 0;						//重新开始
			}	
		}else{										//没有超时直接返回
			return;
		}
	}else if(this->state == ESP8266_WAIT){			//等待状态，空闲等待不执行任何操作，相当于异步延时
		if(HAL_GetTick() >= this->wait_overtime){	//等待结束状态，切换为空闲
			this->state = ESP8266_IDEL;
			this->wait_overtime = 0;
		}else{
			return;
		}
	}

	//设备空闲后每条命令之间需要等待一定的时间间隔
	if(this->state == ESP8266_IDEL){
		if(this->ack_state == 0){			//复位命令发送后需要延时2000ms
			this->state = ESP8266_WAIT;
			this->wait_overtime = HAL_GetTick() + 2000;
			this->ack_state = 0xff;			//重置命令响应状态为：无命令
			return;
		}else if(this->ack_state < 20){	//复位命令发送后需要延时1000ms
			this->state = ESP8266_WAIT;
			this->wait_overtime = HAL_GetTick() + 1000;
			this->ack_state = 0xff;			//重置命令响应状态为：无命令
			return;
		}else{		//完全空闲
			this->retry_time = 0;
		}
	}
	
	/*ESP8266 状态机处理*/
	if(this->step < 10){
		if(this->mode == AP_TCP){
			switch(this->step){
				case 0: esp8266_at_cmd(this, ESP_CMD_RST, OK_ACK, 3000); 		break;	//复位
				case 1: esp8266_at_cmd(this, ESP_CMD_MODE_AP, OK_ACK, 1000); 	break;	//AP 模式
				case 2: esp8266_at_cmd(this, ESP_CMD_CWSAP, OK_ACK, 5000);		break;	//设置WIFI名称和密码
				case 3: esp8266_at_cmd(this, ESP_CMD_CIPAP, OK_ACK, 1000); 	break;	//设置AP的IP地址
				case 4: esp8266_at_cmd(this, ESP_CMD_CIPMODE, OK_ACK, 1000); 	break;	//关闭透传
				case 5: esp8266_at_cmd(this, ESP_CMD_CIPMUX, OK_ACK, 1000); 	break;	//设置多连接
				case 6: esp8266_at_cmd(this, ESP_CMD_CIPSERVER, OK_ACK, 1000); break;	//设置端口号
				default: 
					this->step = 10;
					my_log.show("ESP8266 AP INIT OK!", ESP8266_LOG_SHOW_TIME);
					break;
			}
		}else{
			switch (this->step){
				case 0: esp8266_at_cmd(this, ESP_CMD_RST, OK_ACK, 3000); 		break;	//复位
				case 1: esp8266_at_cmd(this, ESP_CMD_MODE_STA, OK_ACK, 1000); 	break;	//STA模式
				case 2: esp8266_at_cmd(this, ESP_CMD_QAP, OK_ACK, 1000);		break;	//断开连接
				case 3: esp8266_at_cmd(this, ESP_CMD_JAP, "WIFI CONNECTED\r\nWIFI GOT IP\r\n", 3000); break;	//加入wifi 
				default: 
					this->step = 10;	
					my_log.show("ESP8266 STA CONNECT OK!", ESP8266_LOG_SHOW_TIME);
					break;			
			}
		}		
	}else if(this->step < 20){
		if(this->mode == STA_MQTT_ALICLOUD){
			memset(this->send_buf, 0, sizeof(this->send_buf));
			switch(this->step){
				case 10: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ALICLOUD_MQTT_PARAM, this->mqtt_dev_name, this->mqtt_key, this->mqtt_dev_secret); 	break;	//配置MQTT参数
				case 11: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_MQTT_CLIENTID, this->mqtt_key, this->mqtt_dev_name); 								break;	//配置客户端ID 阿里云
				case 12: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ALICLOUD_MQTT_CON, this->mqtt_key);												 	break;	//连接mqtt网络
				case 13: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ALICLOUD_MQTT_POST_SUB, this->mqtt_key, this->mqtt_dev_name); 					 	break;	//订阅 POST 主题
				case 14: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ALICLOUD_MQTT_SET_SUB, this->mqtt_key, this->mqtt_dev_name);					 	break;	//订阅 SET 主题
				default:
					this->step = 20; 
					this->connect = ESP8266_MQTT_CONNECTED;
					my_log.show("ALICLOUD CONNECT OK!", ESP8266_LOG_SHOW_TIME);
					break;
			}
			if(this->step == 11) 	esp8266_at_cmd(this, this->send_buf, OK_ACK, 3000);
			else					esp8266_at_cmd(this, this->send_buf, OK_ACK, 1000);
		}else if(this->mode == STA_MQTT_ONENET){
			memset(this->send_buf, 0, sizeof(this->send_buf));
			switch(this->step){
				case 10: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ONENET_MQTT_PARAM, this->mqtt_dev_name, this->mqtt_key, this->mqtt_dev_secret);  	break;	//配置MQTT参数
				case 11: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ONENET_MQTT_CON); 								 									break;	//连接mqtt网络
				case 12: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ONENET_MQTT_POST_SUB  , this->mqtt_key, this->mqtt_dev_name);						break;	//订阅 POST 主题								 break;	//复位
				case 13: snprintf(this->send_buf, ESP8266_SEND_MAX_LEN, ESP_CMD_ONENET_MQTT_SET_SUB  , this->mqtt_key, this->mqtt_dev_name);					 	break;	//订阅 SET 主题
				default:
					this->step = 20;
					this->connect = ESP8266_MQTT_CONNECTED;				
					my_log.show("ONENET CONNECT OK!", ESP8266_LOG_SHOW_TIME);
					break;
			}
			if(this->step == 11) 	esp8266_at_cmd(this, this->send_buf, OK_ACK, 3000);
			else					esp8266_at_cmd(this, this->send_buf, OK_ACK, 1000);
		}else{
			this->step = 20;
			my_log.show("ESP8266 INIT OK!", ESP8266_LOG_SHOW_TIME);
		}
	}else{

	}

retry:
	return;
}

static int esp8266_at_cmd(s_esp8266* this, const char *cmd, char *expt, uint32_t time_out)
{
	uint32_t rec_len;
	int ret;
    /*检查参数*/
    if(NULL == this || NULL == cmd || expt == NULL){
        return R_NULL;
    }
	/*状态检查错误*/
	if(this->state != ESP8266_IDEL){
		return R_ERROR;
	}

	/*发送命令*/
	ret = my_uart.send(this->uart_id, (uint8_t*)cmd, strlen(cmd));
	if(ret != R_OK)	return R_ERROR;
	
	this->state 		= ESP8266_BUSY;
	this->ack_overtime 	= HAL_GetTick() + time_out;
	this->expt_rec		= expt;
	this->ack_state		= this->step;			//指示当前为哪一条命令的响应
		
	return R_OK;
}





#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
