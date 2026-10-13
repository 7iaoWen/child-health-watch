/********************************************************************************
* @file		:esp8266.h
* @brief	:esp8266.c头文件
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __ESP8266_H__
#define __ESP8266_H__

/*********************STA 连接WIFI  连接命令详解*************************
* Description: 	STA 连接WIFI 串口透传连接命令详解
* Demo  :
STEP1: 设备重启
AT+RST

STEP2:  设计位STA模式
AT+CWMODE=1
 
STEP3: 断开之前的连接
AT+CWQAP

STEP4: 连接WIFI
AT+CWJAP="YOUR_WIFI_SSID","YOUR_WIFI_PASSWORD"
*******************************************************************************/

/*********************阿里云串口透传连接命令详解*************************
* Description: 	阿里云串口透传连接命令详解
* Demo  :
STEP1: 配置连接参数
AT+MQTTUSERCFG=0,1,"NULL","CAR_IN&a1twzbwXv54","3ce2c81dd54d1c3516d8fb7792c50767fbc04e49045ecf9be99e2915ef953b08",0,0,""

STEP2: 配置连接ID		 //clientId  第二个参数,注意每个逗号后加分隔符“\”，已踩过坑，	 //例如：t|securemode=3\,signmethod=hmacsha1\,
 AT+MQTTCLIENTID=0,"a1twzbwXv54.CAR_IN|securemode=2\,signmethod=hmacsha256\,timestamp=1677941302390|"
 
STEP3: 连接阿里云 成功后会在线 
AT+MQTTCONN=0,"a1twzbwXv54.iot-as-mqtt.cn-shanghai.aliyuncs.com",1883,1

STEP4: 订阅主题
AT+MQTTSUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/event/property/post",1		//订阅的主题可在云端设备的“自定义Topic列表”复制进去
AT+MQTTSUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/service/property/set",1

STEP5: 发布消息
AT+MQTTPUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/event/property/post","{\"method\":\"thing.event.property.post\"\,\"id\":\"1234\"\,\"params\":{\"temp\":24}\,\"version\":\"1.0.0\"}",1,0
*******************************************************************************/

/*********************ONENET串口透传连接命令详解*************************
* Description: 	ONENET串口透传连接命令详解
* Demo  :
STEP1: 配置连接参数
AT+MQTTUSERCFG=0,1,"001","YOUR_ONENET_PRODUCT_ID","version=2018-10-31&res=products%2FYOUR_ONENET_PRODUCT_ID%2Fdevices%2F001&et=2018189620&method=md5&sign=YOUR_DEVICE_TOKEN",0,0,""

STEP2: 连接ONENET 成功后会在线 
AT+MQTTCONN=0,"mqtts.heclouds.com",1883,1

STEP3: 订阅主题
AT+MQTTSUB=0,"$sys/YOUR_ONENET_PRODUCT_ID/001/thing/property/post/reply",1
AT+MQTTSUB=0,"$sys/YOUR_ONENET_PRODUCT_ID/001/thing/property/set",1

STEP4: 发布消息
AT+MQTTPUB=0,"$sys/YOUR_ONENET_PRODUCT_ID/001/thing/property/post","{\"id\":\"123\",\"params\":{\"temp\":{\"value\":23}}}",0,0
AT+MQTTPUB=0,"$sys/YOUR_ONENET_PRODUCT_ID/001/thing/property/post","{\"id\":\"123\"\,\"params\":{\"temp\":{\"value\":23}\,\"humi\":{\"value\":50.8\,\"fire\":{\"value\":0}}}",0,0

STEP5: 接收消息
+MQTTSUBRECV:0,"$sys/YOUR_ONENET_PRODUCT_ID/001/thing/property/set",50,{"id":"4","version":"1.0","params":{"fire":false}}
*******************************************************************************/

/* Public Macros  -------------------------------------------------------------------*/
#define	ESP8266_LOG_SHOW_TIME		1000
#define ESP8266_MAX_RETRY_TIMES		5			//定义 ESP8266 的失败最大重发次数
#define	OK_ACK						"OK\r\n"

//AP模式下创建WIFI和服务器
#define ESP_CMD_MODE_AP  	 	"AT+CWMODE=2\r\n"									//AP模式
#define ESP_CMD_CWSAP  		 	"AT+CWSAP=\"YOUR_WIFI_SSID_ap\",\"YOUR_WIFI_PASSWORD\",5,4\r\n"	//创建热点
#define ESP_CMD_CIPMODE  		"AT+CIPMODE=0\r\n"									//关闭透传模式
#define ESP_CMD_CIPMUX  		"AT+CIPMUX=1\r\n"									//设置多连接
#define ESP_CMD_CIPSERVER  		"AT+CIPSERVER=1,8080\r\n"							//设置端口号
#define ESP_CMD_CIPAP  		 	"AT+CIPAP=\"192.168.0.1\"\r\n"						//开启服务器
#define ESP_CMD_TCP_SEND  		"AT+CIPSEND=0,%d\r\n"						//开启服务器

//STA模式下连接WIFI
#define ESP_CMD_RST    	 		"AT+RST\r\n"      	//复位
#define ESP_CMD_MODE_STA  	 	"AT+CWMODE=1\r\n"
#define ESP_CMD_QAP 		 	"AT+CWQAP\r\n"
#define ESP_CMD_JAP  		 	"AT+CWJAP=\"YOUR_WIFI_SSID\",\"YOUR_WIFI_PASSWORD\"\r\n"

//配置MQTT连接参数
#define ESP_CMD_ALICLOUD_MQTT_PARAM 			"AT+MQTTUSERCFG=0,1,\"NULL\",\"%s&%s\",\"%s\",0,0,\"\"\r\n"		//阿里云 //MQTT YOUR_PRODUCT_KEY|securemode=2,signmethod=hmacsha256,timestamp=1678008992524|
#define ESP_CMD_ONENET_MQTT_PARAM 		  		"AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n"					//onenet

//配置客户端ID 阿里云才需要
#define ESP_CMD_MQTT_CLIENTID 					"AT+MQTTCLIENTID=0,\"%s.%s|securemode=2\\\,signmethod=hmacsha256\\\,timestamp=1728701893669|\"\r\n"	 //注意这里要改时间戳 在MQTT连接参数里面

//连接MQTT服务器
#define ESP_CMD_ALICLOUD_MQTT_CON 				"AT+MQTTCONN=0,\"%s.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883,1\r\n"			//阿里云
#define ESP_CMD_ONENET_MQTT_CON 				"AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n"												//onenet

//断开MQTT连接
#define ESP_CMD_MQTT_DISCON 					"AT+MQTTCLEAN=0\r\n"

//订阅POST主题
#define ESP_CMD_ALICLOUD_MQTT_POST_SUB 			"AT+MQTTSUB=0,\"/sys/%s/%s/thing/event/property/post\",1\r\n"
#define ESP_CMD_ONENET_MQTT_POST_SUB 			"AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/post/reply\",1\r\n"

//订阅SET主题
#define ESP_CMD_ALICLOUD_MQTT_SET_SUB 			"AT+MQTTSUB=0,\"/sys/%s/%s/thing/service/property/set\",1\r\n"
#define ESP_CMD_ONENET_MQTT_SET_SUB 			"AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/set\",1\r\n"

//发布消息
#define ESP_CMD_ONENET_MQTT_PUB 				"AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{%s}}\",0,0\r\n"		//onenet
#define ESP_CMD_ALICLOUD_MQTT_PUB 				"AT+MQTTPUB=0,\"/sys/%s/%s/thing/event/property/post\",\"{\\\"params\\\":{%s}\\\,\\\"version\\\":\\\"1.0.0\\\"}\",0,0\r\n"	//阿里云


/* Public Typedef  -------------------------------------------------------------------*/
/*状态机*/
typedef enum
{
	ESP8266_UNINITED = 0,        					//未初始化
	ESP8266_IDEL,									//空闲状态 
	ESP8266_BUSY,									//繁忙状态
	ESP8266_WAIT,									//空闲等待状态
}esp_sta_t;

/*ESP8266 连接状态*/
typedef enum
{
	ESP8266_DISCONNECT = 0,					//无任何连接
	ESP8266_WIFI_CONNECTED,					//已建立WIFI连接
	ESP8266_TCP_CONNECTED,        			//已建立 TCP 连接
	ESP8266_MQTT_CONNECTED,					//已建立 MQTT 连接
}esp_connect_sta_t;

/*模式选择*/
typedef enum
{
	AP_TCP = 0,										//AP模式下的TCP通信
	STA_TCP,										//STA模式下的TCP通信
	STA_MQTT_ALICLOUD,								//STA模式下的MQTT通信 + 阿里云
	STA_MQTT_ONENET,								//STA模式下的MQTT通信 + ONENET
}work_mode_t;
/** 
  * @brief ESP8266类，结构体定义 
  */
typedef struct _ESP8266 c_esp8266;
typedef struct _ESP8266
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;

    int(*state)  (const c_esp8266* p_obj);
    /************************************************* 
	* Description: esp8266 mqtt post 上报数据
	* Input : <p_obj> esp8266对象
			  <data> 待发送的数据
	* Demo  :
		//MQTT 云平台模式
		if(esp8266.state(&esp8266) == ESP8266_MQTT_CONNECTED){
			memset(tx_buf, 0, sizeof(tx_buf));
			sprintf(tx_buf,"\\\"temp\\\":%d\\,\\\"o2\\\":%d\\,\\\"heart\\\":%d", temp, o2, heart);			   //阿里云
			sprintf(tx_buf,"\\\"temp\\\":{\\\"value\\\":%d}\\,\\\"humi\\\":{\\\"value\\\":%d}", temp, humi);	//onenet
			ret = esp8266.esp_send(&esp8266, tx_buf);  				
		}
		//TCP 模式
		if(esp8266.state(&esp8266) == ESP8266_TCP_CONNECTED){
			memset(tx_buf, 0, sizeof(tx_buf));
			sprintf(tx_buf,"AT+DATA=100,50\r\n");
			ret = esp8266.esp8266_send(&esp8266, tx_buf);  				
		}
	*************************************************/
	int (*send)(const c_esp8266* p_obj, const char *data);
    /************************************************* 
	* Description: esp8266 注册 ESP8266的 接收回调函数，MQTT模式下为云平台的接收回调函数，TCP模式下为服务器数据的接收回调函数
	* Input : <p_obj> esp8266对象
			  <callback> 待发送的数据
			  <param> 待发送的数据
	* Demo  :
	int esp8266_recive_callback(void* param, char* key, char *value){return R_OK;}
	esp8266.set_recive_callback(&esp8266, esp8266_recive_callback, NULL); 
	*************************************************/
    int (*set_recive_callback) (const c_esp8266* p_obj, int(*callback)(void* param, char* key, char* value), void *param);
}c_esp8266;

/************************************************* 
* Description: 创建 esp8266 对象  (创建了，获取状态为已连接的话可以直接发送)
* Input : <uart_id> 串口编号， MY_UART1 - MY_UART3;
* Return:  esp8266 wifi 模块 对象
* Demo  :

//MQTT ONENET 云平台模式
c_esp8266 esp8266 = esp8266_creat(MY_UART3,  //选择串口3
       STA_MQTT_ONENET, //工作模式: MQTT + ONENET
       "YOUR_ONENET_PRODUCT_ID",  //产品ID
       "001",    //设备名
       "version=2018-10-31&res=products%2FYOUR_ONENET_PRODUCT_ID%2Fdevices%2F001&et=2018189620&method=md5&sign=YOUR_DEVICE_TOKEN"); //密码 需要在MQTT连接参数里面找
esp8266.set_recive_callback(&esp8266, esp8266_recive_callback, NULL);  

//MQTT 阿里云 模式
c_esp8266 esp8266 = esp8266_creat(MY_UART3,   //选择串口3
        STA_MQTT_ALICLOUD, //工作模式:MQTT + ALICLOUD
        "YOUR_PRODUCT_KEY",  //产品ID
        "stm32",   //设备名
        "YOUR_DEVICE_SECRET"); 
esp8266.set_recive_callback(&esp8266, esp8266_recive_callback, NULL);  

//TCP模式
c_esp8266 esp8266 = esp8266_creat(MY_UART3,   //选择串口3
    AP_TCP,    //工作模式: TCP模式
    "",        //产品ID
    "",        //设备名
    ""); 
esp8266.set_recive_callback(&esp8266, esp8266_recive_callback, NULL); 

*************************************************/
c_esp8266 esp8266_creat(my_uart_id_t uart_id, work_mode_t mode, const char *key, const char *dev_name, const char *dev_secret);

#endif 

/*数据缓存处理demo*/
//	static char rec_buf[64];
//	static uint16_t data_len;
//	data_len = strlen(value);
//	if(data_len < 64){
//		memset(rec_buf, 0, sizeof(rec_buf));
//		memcpy(rec_buf, value, data_len);
//	}else{
//		my_log.show("ESP OVER LENTH");
//	}	
//	
//	char *pxIndex;
//	if(strstr(rec_buf,"\r\n") != NULL){
//		if(strstr(rec_buf, "AT+DATA") != NULL){
//		}
//	}
