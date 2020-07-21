/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
/****************************************************************************
*
* This demo showcases BLE GATT client. It can scan BLE devices and connect to one device.
* Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include <math.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "sys/unistd.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include"cJSON.h"
/*************************smartconfig p配网相关头文件*************************/
#include "esp_sleep.h"
#include <errno.h>
#include "esp_sntp.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_smartconfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_ota_ops.h"
#include <sys/socket.h>
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"

//I2C 
#define I2C_SCL_IO          33                  //SCL->IO33
#define I2C_SDA_IO          32                  //SDA->IO32
#define I2C_MASTER_NUM      I2C_NUM_1           //I2C_1
#define WRITE_BIT           I2C_MASTER_WRITE    //写:0
#define READ_BIT            I2C_MASTER_READ     //读:1
#define ACK_CHECK_EN        0x1                 //主机检查从机的ACK
#define ACK_CHECK_DIS       0x0                 //主机不检查从机的ACK
#define ACK_VAL             0x0                 //应答
#define NACK_VAL            0x1                 //不应答
#define  LED_IO        21
//SHT30
#define SHT30_WRITE_ADDR    0x44                //地址 
#define CMD_FETCH_DATA_H    0x22                //循环采样，参考sht30 datasheet
#define CMD_FETCH_DATA_L    0x36
#define CID_ESP 0x02E5
//uart  gpio引脚
#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0
#define Device_ID  "SWC20200520000120210520"
TaskHandle_t led_test_task_Handler,http_test_task_Handler,peripheral_get_task_Handler;
static SemaphoreHandle_t gattc_semaphore;//定义二值信号量
static SemaphoreHandle_t notify_semaphore;
const char *webaddress1[3]={"http://47.101.166.0:20005/originalMonitor.action","http://47.101.166.0:20005/monitor.action",0};
unsigned char sht30_buf[6]={0}; //温湿度变量
float g_temp=0.0, g_rh=0.0,pm=0.0;
uint8_t  pm_rateH,pm_rateL;
static const int RX_BUF_SIZE = 1024;//定义接收缓冲区大小
#define   SAVE_POWER_WAIT_TIME     60//180S
static RTC_DATA_ATTR struct timeval sleep_enter_time;
short http_send_flag=0,low_power_flag=0;
char wifi_flag=0;
char http_send_fail=0;
short person_flag=0,server_data_flag=0;
int Tmp_Hum_Time=0;
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
//wifi函数
static EventGroupHandle_t  wifi_event_group;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG2 = "wifi station";
static int s_retry_num = 0;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG1 = "smartconfig_example";
void smartconfig_example_task(void *parm);
/*****************网络状态指示灯**************/
static void wifi_led_test_task(void *pvParameters);//wifi断开
static void led_test_task(void *pvParameters);
static const char *TAGTIME = "TIME";
static void obtain_time(void);
static void peripheral_init(void);
static void peripheral_get_task(void);
char *cname;
#define false		0
#define true		1
#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "HTTP_CLIENT";
    nvs_handle handle;
static const char *NVS_CUSTOMER = "customer data";
static const char *DATA1= "param 1";
int  HTTP_Send_Data[20]={0};
float environment[3]={0};
static const char remote_device_name[] = "ESP_GATTS_DEMO";
static bool connect1    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;
static bool is_connecet = false;
/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
uint8_t i=0;
static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};
static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};
static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};
struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;//gatt客户端回调回调函数
    uint16_t gattc_if;//此文配置Gatt客户端接口号
    uint16_t app_id;//应用程序配置文件id号
    uint16_t conn_id;//链接id
    uint16_t service_start_handle;//服务启动句柄
    uint16_t service_end_handle;//服务结束句柄
    uint16_t char_handle;//字符处理
    esp_bd_addr_t remote_bda;//链接到此客户的远程设备地址
};
/*IIC初始化*/
void i2c_init(void)
{
	//i2c配置结构体
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;                    //I2C模式
    conf.sda_io_num = I2C_SDA_IO;                   //SDA IO映射
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;        //SDA IO模式
    conf.scl_io_num = I2C_SCL_IO;                   //SCL IO映射
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;        //SCL IO模式
    conf.master.clk_speed = 100000;                 //I2C CLK频率
    i2c_param_config(I2C_MASTER_NUM, &conf);        //设置I2C
    //注册I2C服务即使能
    i2c_driver_install(I2C_MASTER_NUM, conf.mode,0,0,0);
}
/*
* sht30初始化
* @param[in]   void  		        :无
* @retval      int                  :0成功，其他失败
*/
int sht30_init(void)
{
    int ret;
    //配置SHT30的寄存器
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                                   //新建操作I2C句柄
    i2c_master_start(cmd);                                                          //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);    //发地址+写+检查ack
    i2c_master_write_byte(cmd, CMD_FETCH_DATA_H, ACK_CHECK_EN);                     //发数据高8位+检查ack
    i2c_master_write_byte(cmd, CMD_FETCH_DATA_L, ACK_CHECK_EN);                     //发数据低8位+检查ack
    i2c_master_stop(cmd);                                                           //停止I2C
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);        //I2C发送
    i2c_cmd_link_delete(cmd);                                                       //删除I2C句柄
    return ret;
}

/*
* sht30校验算法
*/
unsigned char SHT3X_CalcCrc(unsigned char *data, unsigned char nbrOfBytes)
{
	unsigned char bit;        // bit mask
    unsigned char crc = 0xFF; // calculated checksum
    unsigned char byteCtr;    // byte counter
    unsigned int POLYNOMIAL =  0x131;           // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

    // calculates 8-Bit checksum with given polynomial
    for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
        crc ^= (data[byteCtr]);
        for(bit = 8; bit > 0; --bit) {
            if(crc & 0x80) {
                crc = (crc << 1) ^ POLYNOMIAL;
            }  else {
                crc = (crc << 1);
            }
        }
    }
	return crc;
}
/*
* sht30数据校验
*/
unsigned char SHT3X_CheckCrc(unsigned char *pdata, unsigned char nbrOfBytes, unsigned char checksum)
{
    unsigned char crc;
	crc = SHT3X_CalcCrc(pdata, nbrOfBytes);// calculates 8-Bit checksum
    if(crc != checksum) 
    {   
        return 1;           
    }
    return 0;              
}

int sht30_get_value(void)
{
    int ret;
    //配置SHT30的寄存器
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                                   //新建操作I2C句柄
    i2c_master_start(cmd);                                                          //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | READ_BIT, ACK_CHECK_EN);     //发地址+读+检查ack
    i2c_master_read_byte(cmd, &sht30_buf[0], ACK_VAL);                               //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[1], ACK_VAL);                               //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[2], ACK_VAL);                               //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[3], ACK_VAL);                               //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[4], ACK_VAL);                               //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[5], NACK_VAL);                              //读取数据+不回复ack
    i2c_master_stop(cmd);                                                            //停止I2C
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);         //I2C发送
    if(ret!=ESP_OK)
    {
        return ret;
    }
    i2c_cmd_link_delete(cmd);                                                       //删除I2C句柄
    //校验读出来的数据，算法参考sht30 datasheet
    if( (!SHT3X_CheckCrc(sht30_buf,2,sht30_buf[2])) && (!SHT3X_CheckCrc(sht30_buf+3,2,sht30_buf[5])) )
    {
        ret = ESP_OK;//成功
    }
    else
    {
        ret = 1;
    }
    return ret;
}
void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
/*创建一个接收数据函数*/
static void rx_task(void *arg)//创建接收数据函数
{
    uint8_t senddata[9] = {0xff,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};//发送请求命令
    uart_write_bytes(UART_NUM_1, (char *)senddata, 9);//进行数据发送
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);//读取串口数据
        if (rxBytes > 0) {//判断数据是否接收成功
            data[rxBytes] = 0;//在接收数据末尾加上停止校验位
            pm_rateH=data[2];
            pm_rateL=data[3];
            pm=pm_rateH<<8|pm_rateL;
           vTaskDelay(100/portTICK_RATE_MS);
          }
    free(data);
    vTaskDelete(NULL);
}
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");
void Get_CurrentData(char *dat)
{
                         time_t now;
                     struct tm timeinfo;
                     time(&now);
                     localtime_r(&now, &timeinfo);
                     if (timeinfo.tm_year < (2016 - 1900)) {
                     obtain_time();
                     time(&now);
                     }
                    char strftime_buf[64];
                    setenv("TZ", "CST-8", 1);
                    tzset();
                    localtime_r(&now, &timeinfo);
                    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
                    cname=strftime_buf;
     cJSON *root=NULL;//定义一个指针变量
     root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 儿童
    cJSON_AddNumberToObject(root,"breathFlag",HTTP_Send_Data[0]);//呼吸频率
    cJSON_AddNumberToObject(root,"breathRate",HTTP_Send_Data[1]);//呼吸频率
    cJSON_AddNumberToObject(root,"heartRate",HTTP_Send_Data[2]);//心率
    cJSON_AddNumberToObject(root,"bodyTP",HTTP_Send_Data[3]);//体温
    cJSON_AddNumberToObject(root,"envTP",environment[0]);//环境温度
    cJSON_AddNumberToObject(root,"envRH",environment[1]);//环境湿度
    cJSON_AddNumberToObject(root,"envPM",environment[2]);//环境PM2.5
    cJSON_AddStringToObject(root,"pushTime",cname);
   
     char *out=cJSON_PrintUnformatted(root);
     memcpy(dat,out,strlen(out));//将整合的数据copy给dat
     cJSON_Delete(root);
     free(out);
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER://获取请求头文件事件
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
           ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
             printf("%.*s   \n", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_FINISH:
           ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
           ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
           if (err != 0) {
               ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

static void http_rest_with_url(void)
{ 
    esp_http_client_config_t config = {
        .url = webaddress1[1],
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);//用户客户端初始化
    char post_data1[1024]={0};
    Get_CurrentData(post_data1);//得到json数据，用如post data
    	printf("%s\n",post_data1);
    // POST
     esp_http_client_set_url(client, webaddress1[1]);//接口地址
     esp_http_client_set_method(client, HTTP_METHOD_POST);//请求方法
     esp_http_client_set_post_field(client, post_data1, strlen(post_data1));//设置发送格式，获取json数据
     esp_err_t  err = esp_http_client_perform(client);//完成tcp的发送和接收
    if (err == ESP_OK) { 
         ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));             
        gpio_set_level(LED_IO, 0);
        vTaskDelay(30 / portTICK_PERIOD_MS);///freertos里面的延时函数
        gpio_set_level(LED_IO, 1);
        vTaskDelay(30 / portTICK_PERIOD_MS);
         gpio_set_level(LED_IO, 0);
        vTaskDelay(30 / portTICK_PERIOD_MS);///freertos里面的延时函数
        gpio_set_level(LED_IO, 1);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));//http 发送数据失败，可能是网络断了，也可能使服务器问题
        xTaskCreate(&wifi_led_test_task, "led_test_task", 2048, NULL, 8, NULL);//数据发送不成功 ，灯快速闪动

    }
    esp_http_client_cleanup(client);
}
static void http_test_task(void *pvParameters)
{ 
    http_rest_with_url();
    vTaskDelete(NULL);
}

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)//wifi事件组
{  nvs_handle handle;
    esp_err_t err;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    wifi_config_t wifi_config_stored;
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    uint32_t len = sizeof(wifi_config_stored);
    	// Open
    err =nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {//第一步执行
         err = nvs_get_blob(handle, DATA1, &wifi_config_stored, &len);
	    if (err == ESP_ERR_NVS_NOT_FOUND) 
		{wifi_flag=0;
		 xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
		}
		else if(err == ESP_OK)
		{wifi_flag=1;
           esp_wifi_connect();
           nvs_close(handle);
		}
        //创建smartconfig任务
       
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if(wifi_flag==1)
        {
               if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                 /****************清除nvs_flash*************/
             ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
             ESP_ERROR_CHECK(  nvs_erase_key(handle,DATA1));
             vTaskDelay(1000 / portTICK_PERIOD_MS);
             ESP_ERROR_CHECK( nvs_erase_all(handle));
             vTaskDelay(1000 / portTICK_PERIOD_MS);
             ESP_ERROR_CHECK( nvs_commit(handle) );
             vTaskDelay(1000 / portTICK_PERIOD_MS);
             nvs_close(handle); 
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
              }
    
        ESP_LOGI(TAG,"connect to the AP fail");
        }else{//断线重连  密码错误尝试重新链接
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);//清除标志位
        ESP_LOGI(TAG, "断线重连.........");
         esp_restart();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {//获取到wifi信息后，执行， 第七步
             if(wifi_flag==1)
                      {
                           ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                           ESP_LOGI(TAG2, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                           s_retry_num = 0;
                           xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                      }else{
                               //sta链接成功，set事件组
                               xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);}
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {//等待配网 第四步，等待手机配网
        ESP_LOGI(TAG1, "Scan done");
    xTaskCreate(&led_test_task, "led_test_task", 2048, NULL, 7, &led_test_task_Handler); 
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {//扫描信道 第五步，找到udp广播
        ESP_LOGI(TAG1, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) { //获取到ssid和密码   第六步，获取 wifi信息
        ESP_LOGI(TAG1, "Got SSID and password");
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        //打印账号密码
        ESP_LOGI(TAG1, "SSID:%s", ssid);
        ESP_LOGI(TAG1, "PASSWORD:%s", password);
        //断开默认的
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
          //设置获取的ap和密码到寄存器
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        //将获取的密码保存起来 nvs_flash
         ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
         ESP_ERROR_CHECK( nvs_set_blob( handle, DATA1, &wifi_config, sizeof(wifi_config)) );
         ESP_ERROR_CHECK( nvs_commit(handle) );
         nvs_close(handle);
        //连接获取的ssid和密码
        ESP_ERROR_CHECK( esp_wifi_connect() );
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}
/*初始化 wifi*/
static void initialise_wifi(void)
{    
    wifi_event_group = xEventGroupCreate();//创建一个事件组
     s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());//wifi事件
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );  //获取链接信息
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );//获取链接信息
    if(wifi_flag!=1)ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );     //获取链接信息
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );//设置模式
    if(wifi_flag==1){
     nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    wifi_config_t wifi_config_stored;
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    uint32_t len = sizeof(wifi_config_stored);
    ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA1, &wifi_config_stored, &len) );
    nvs_close(handle);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_stored) );
    wifi_flag=1;}
    ESP_ERROR_CHECK( esp_wifi_start() );//启动wifi
            ESP_LOGI(TAG, "wifi_init_sta finished.");
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
   
    if (bits & WIFI_CONNECTED_BIT) {
/**************************wifi链接成功后，进行一次sntp时间戳初始化************************************/
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
     if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAGTIME, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    char strftime_buf[64];
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);
/**************************wsntp时间戳初始化结束************************************/
    } else if (bits & WIFI_FAIL_BIT) {
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT"); 
    }
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
/*smartconfig任务*/
void smartconfig_example_task(void *parm)
{
    EventBits_t uxBits;
    //使用ESP-TOUCH配置
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));//第二步执行
    //开始sc
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();//建立长链接，获取wifi信息
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1)
    { //死等事件组：CONNECTED_BIT | ESPTOUCH_DONE_BIT，第三步执行，死等，等待事件组
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
            //连上ap
        if (uxBits & CONNECTED_BIT) //第八步
        { 
            ESP_LOGI(TAG1, "WiFi Connected to ap");
        }
        //sc结束
        if (uxBits & ESPTOUCH_DONE_BIT)//第九步
        { ESP_LOGI(TAG1, "smartconfig over");
            vTaskDelete(led_test_task_Handler);
            esp_smartconfig_stop();
            esp_restart();



            vTaskDelete(NULL);   
         }}}
/******************smartconfig   over************************************************/
/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{//gatt   轮廓文件事件
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event) {
    case ESP_GATTC_REG_EVT://gatt注册
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);//得到抓包信息参数，扫描adv完成
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:{//gatt连接成功事件//链接成功后第一步
        is_connecet = true;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT://gatt open事件
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT://未发现设备事件
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_CFG_MTU_EVT://配置mtu事件  第二步执行
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {//找寻 回应
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "service found");
            get_server = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT://找寻成功，得到设备信息
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }
                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT://接收广告通知
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");//执行这句
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        
        server_data_flag=1;
        Tmp_Hum_Time++;
       esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);  //将接收到数据打印出来
            HTTP_Send_Data[0]=p_data->notify.value[0];//将接收到的数据 传递给http
            HTTP_Send_Data[1]=p_data->notify.value[1];
            HTTP_Send_Data[2]=p_data->notify.value[2];
            HTTP_Send_Data[3]=p_data->notify.value[3];
            xTaskCreate(&peripheral_get_task, "peripheral_get_task", 8192, NULL, 10, &peripheral_get_task_Handler);//高优先级通过获取二值信号量

             xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 11, &http_test_task_Handler);//高优先级通过获取二值信号量
  

        break;
    case ESP_GATTC_WRITE_DESCR_EVT://写描述事件//由client写数据到server端，写数据成功   demo执行
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");//由client写数据到server端，写数据成功
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i+10 ;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");//由client写数据到server端，写数据成功
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        is_connecet = false;//链接标志位
        connect1 = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);//开始扫描
            ESP_LOGI(GATTC_TAG, "time 1");
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)/////查找所有发出广播的设备，GAP
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);//开始扫描
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {//扫描成功或失败
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");//开始扫描
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {//扫描结果
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT://查找所有发出广播的设备
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");

            if (adv_name != NULL) {
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (connect1 == false) {
                        connect1 = true;
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                    }
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT://可以用重新启动扫描 
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT://接收公共参数数据
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) { 
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;//gl_profile_tab     定义的结构体指针变量，gatt后进行函数回调
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}


void app_main(void)
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    initialise_wifi();
     peripheral_init();//外设初始化（温湿度　pm2.5等）
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);//设置模式BLE
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();//BLE堆栈初始化
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);//状态机，进行gap函数回调
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);//状态机，进行gatt函数回调 
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);//状态机，进行手机函数回调 
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
        notify_semaphore = xSemaphoreCreateBinary();
    if (!notify_semaphore) {
        ESP_LOGE(GATTC_TAG, "%s, init fail, the notify semaphore create fail.", __func__);
        return;
    }
      gattc_semaphore = xSemaphoreCreateBinary();
    if (!gattc_semaphore) {
        ESP_LOGE(GATTC_TAG, "%s, init fail, the gattc semaphore create fail.", __func__);
        return;
    }
}

static void obtain_time(void)
{

      ESP_LOGI(TAGTIME, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "cn.ntp.org.cn");
    sntp_setservername(1, "210.72.145.44");		// 国家授时中心服务器 IP 地址
    sntp_setservername(2, "1.cn.pool.ntp.org");        
    sntp_init();
    setenv("TZ", "CST-8", 1);
    tzset();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAGTIME, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);//获取网络时间，64bit的秒计数
    localtime_r(&now, &timeinfo);//转化为具体的时间参数
}

   static void led_test_task(void *pvParameters)
{
    while(1){
    
            gpio_set_level(LED_IO, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_IO, 0);  
            vTaskDelay(1000 / portTICK_PERIOD_MS);///freertos里面的延时函数
        
    }
    vTaskDelete(NULL);
   
}

static void wifi_led_test_task(void *pvParameters)//wifi断开
{
 
            gpio_set_level(LED_IO, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数  
             gpio_set_level(LED_IO, 0);  
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数
              gpio_set_level(LED_IO, 1);
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_IO, 0);  
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数
             gpio_set_level(LED_IO, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数  
             gpio_set_level(LED_IO, 0);  
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数
              gpio_set_level(LED_IO, 1);
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_IO, 0);  
             vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数
           
            vTaskDelete(NULL);
   
}

static void peripheral_get_task(void)
{      
    
       if(sht30_get_value()==ESP_OK)   //获取温湿度
        {
            //算法参考sht30 datasheet
            g_temp    =( ( (  (sht30_buf[0]*256) +sht30_buf[1]) *175   )/65535.0  -45  );
            g_rh  =  ( ( (sht30_buf[3]*256) + (sht30_buf[4]) )*100/65535.0) ;
            ESP_LOGI("SHT30", "temp:%4.2f C \r\n", g_temp); //℃打印出来是乱码,所以用C
            ESP_LOGI("SHT30", "hum:%4.2f %%RH \r\n", g_rh);
        }  
         xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, 9, NULL); 
         ESP_LOGI("PM2.5", "pm:%4.2f %%ug/m3 \r\n", pm); 
         vTaskDelay(100 / portTICK_PERIOD_MS);
         environment[0]=(int)g_temp;
         environment[1]=(int)g_rh;
         environment[2]=pm;
         vTaskDelete(NULL);
}
static void peripheral_init(void)
{  
     uart_init(); 
    i2c_init();                         //I2C初始化
    sht30_init();                       //sht30初始化
    vTaskDelay(100/portTICK_RATE_MS);   //延时100ms 
    gpio_pad_select_gpio(LED_IO); 
    gpio_set_direction(LED_IO, GPIO_MODE_OUTPUT);
   
}