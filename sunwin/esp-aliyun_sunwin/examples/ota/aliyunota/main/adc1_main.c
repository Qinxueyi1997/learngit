#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "sys/unistd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "sys/unistd.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include"cJSON.h"
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
/*******************在线ota**************************************/
#include "esp_ota_ops.h"
#include <sys/socket.h>
#include "infra_compat.h"
#include "ota_solo.h"
#include "conn_mgr.h"
/**********************************低功耗相关头文件**********************/
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
static bool ota_started = false;
#define   SAVE_POWER_WAIT_TIME    3000//5分钟
#define Device_ID  "SWA20200603000120210603"
const char *address;
const char *webaddress[3]={"http://47.101.166.0:20005/originalMonitor.action","http://47.101.166.0:20005/monitor.action",0};
TaskHandle_t low_power_Handler;
static RTC_DATA_ATTR struct timeval sleep_enter_time;
static SemaphoreHandle_t adc_semaphore,original_semaphore;
uint32_t num1=0,num2=0,num3=0;
uint8_t time1=0;
 char post_data2[1024]={0};
/**********************************低功耗相关文件**********************/
static void low_vol_protect_task(void *arg)
{   
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT1: {
            uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
            if (wakeup_pin_mask != 0) {
                int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
                printf("Wake up from GPIO %d\n", pin);
            } else {
                printf("Wake up from GPIO\n");
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER: {
            printf("Wake up from timer. Time spent in deep sleep: %dms\n", sleep_time_ms);//定时器打印deep_sleep时间
            break;
        }
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            printf("Not a deep sleep reset\n");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    const int ext_wakeup_pin_2 = 35;
    const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;
     printf("Enabling EXT1 wakeup on pins GPIO%d\n", ext_wakeup_pin_2);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    printf("Entering deep sleep\n");
    gettimeofday(&sleep_enter_time, NULL);
    esp_deep_sleep_start();
     vTaskDelete(NULL);
}

short http_send_heart_flag=0,http_send_flag=0,low_power_flag=0;
char wifi_flag=0;
char http_send_fail=0;
short person_flag=0,http_send_time=0,circle_time=0,sntp_init_flag=0,http_data_type_flag=0;
//http_data_type_flag=1  表示原始数据
//http_data_type_flag=2  表示呼吸心率信息

#define EXAMPLE_ESP_MAXIMUM_RETRY  5
static const char *TAGTIME = "TIME";
static void obtain_time(void);
char *cname;
/************************************************************************************/
/*
===========================
宏定义
=========================== 
*/
#define false		0
#define true		1
#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "HTTP_CLIENT";
    nvs_handle handle;
static const char *NVS_CUSTOMER = "customer data";
static const char *DATA1= "param 1";
int  HTTP_Send_Data[20]={0},original_data[12000]={0};
int HTTP_Body_Data[5]={1,20,0,0,0};
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");
void Get_Current_Original_Data(char *dat)
{
     cJSON *root=NULL;//定义一个指针变量
     root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);//
    cJSON_AddStringToObject(root,"pushTime",cname);
    cJSON *array=NULL;

     cJSON_AddItemToObject(root,"original",array=cJSON_CreateArray());  
    
      cJSON *ArrayItem0=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem0,"pv",HTTP_Send_Data[0]);
      cJSON_AddNumberToObject(ArrayItem0,"pr",HTTP_Send_Data[1]);
    
      cJSON_AddItemToArray(array,ArrayItem0);
      cJSON *ArrayItem1=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem1,"pv",HTTP_Send_Data[2]);
      cJSON_AddNumberToObject(ArrayItem1,"pr",HTTP_Send_Data[3]);
   
      cJSON_AddItemToArray(array,ArrayItem1);
      cJSON *ArrayItem2=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem2,"pv",HTTP_Send_Data[4]);
      cJSON_AddNumberToObject(ArrayItem2,"pr",HTTP_Send_Data[5]);

      cJSON_AddItemToArray(array,ArrayItem2);
      cJSON *ArrayItem3=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem3,"pv",HTTP_Send_Data[6]);
      cJSON_AddNumberToObject(ArrayItem3,"pr",HTTP_Send_Data[7]);

      cJSON_AddItemToArray(array,ArrayItem3);
     cJSON *ArrayItem4=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem4,"pv",HTTP_Send_Data[8]);
      cJSON_AddNumberToObject(ArrayItem4,"pr",HTTP_Send_Data[9]);
      
      cJSON_AddItemToArray(array,ArrayItem4);
      cJSON *ArrayItem5=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem5,"pv",HTTP_Send_Data[10]);
      cJSON_AddNumberToObject(ArrayItem5,"pr",HTTP_Send_Data[11]);
  
      cJSON_AddItemToArray(array,ArrayItem5);
      cJSON *ArrayItem6=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem6,"pv",HTTP_Send_Data[12]);
      cJSON_AddNumberToObject(ArrayItem6,"pr",HTTP_Send_Data[13]);
  
      cJSON_AddItemToArray(array,ArrayItem6);
      cJSON *ArrayItem7=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem7,"pv",HTTP_Send_Data[14]);
      cJSON_AddNumberToObject(ArrayItem7,"pr",HTTP_Send_Data[15]);

      cJSON_AddItemToArray(array,ArrayItem7);
      cJSON *ArrayItem8=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem8,"pv",HTTP_Send_Data[16]);
      cJSON_AddNumberToObject(ArrayItem8,"pr",HTTP_Send_Data[17]);

      cJSON_AddItemToArray(array,ArrayItem8);
      cJSON *ArrayItem9=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem9,"pv",HTTP_Send_Data[18]);
      cJSON_AddNumberToObject(ArrayItem9,"pr",HTTP_Send_Data[19]);  
      cJSON_AddItemToArray(array,ArrayItem9);
     char *out=cJSON_PrintUnformatted(root);
     memcpy(dat,out,strlen(out));//将整合的数据copy给dat
     cJSON_Delete(root);
     free(out);
}

void Get_Current_Body_Data(char *dat)
{
     cJSON *root=NULL;//定义一个指针变量
     root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 成人
    cJSON_AddNumberToObject(root,"breathFlag",HTTP_Body_Data[0]);//状态标志位 1正常 2 呼吸暂停 3低通气 4 呼吸过快 5 不通气
    cJSON_AddNumberToObject(root,"breathRate",HTTP_Body_Data[1]);//呼吸频率
    cJSON_AddNumberToObject(root,"heartRate",HTTP_Body_Data[2]);//心率
    cJSON_AddNumberToObject(root,"bodyTP",HTTP_Body_Data[3]);//体温
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
           //  ESP_LOGI(TAG, "Receivedata=  %s            \n", (char*)evt->data);
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
        .url = address,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);//用户客户端初始化
    char post_data1[1024]={0};
    // POST
    memcpy(post_data1, post_data2,strlen(post_data2));
    esp_http_client_set_url(client, address);//接口地址
    esp_http_client_set_method(client, HTTP_METHOD_POST);//请求方法
      
     esp_http_client_set_post_field(client, post_data1, strlen(post_data1));//设置发送格式，获取json数据
     esp_err_t  err = esp_http_client_perform(client);//完成tcp的发送和接收
    if (err == ESP_OK) { 
         ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        http_send_fail++;
        if(http_send_fail==10)
        {
            http_send_fail=0;
           // esp_restart();
        }
    }
    esp_http_client_cleanup(client);
}
static void http_test_task(void *pvParameters)
{
   
    while(1){
    if(adc_semaphore!=NULL)
    {
     int err = xSemaphoreTake(adc_semaphore, portMAX_DELAY);

    if(err== pdTRUE){
        if(http_data_type_flag==1)
        {
    address = webaddress[0];
    Get_Current_Original_Data(post_data2);//得到json数据，用如post data
        }else{

           address = webaddress[1];
           Get_Current_Body_Data(post_data2);//得到json数据，用如post data


        }
   
    http_rest_with_url();
    
    }}
    
    }
    vTaskDelete(NULL);
}
static void http_body_test_task(void *pvParameters)
{ address = webaddress[1];
    
    http_rest_with_url();
    vTaskDelete(NULL);
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
// 2020.4.14
//ADC采集任务
static void adc_init(void)
{
    esp_err_t r;
    gpio_num_t adc_gpio_num1,adc_gpio_num2, adc_gpio_num3;
    r = adc1_pad_get_io_num( ADC1_CHANNEL_5, &adc_gpio_num1 );
    assert( r == ESP_OK );
     r = adc1_pad_get_io_num( ADC1_CHANNEL_6, &adc_gpio_num2 );
    assert( r == ESP_OK );
     r = adc1_pad_get_io_num( ADC1_CHANNEL_7, &adc_gpio_num3 );
    assert( r == ESP_OK );

    //be sure to do the init before using adc1. 
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_5, ADC_ATTEN_11db);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_6, ADC_ATTEN_11db);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_7, ADC_ATTEN_11db);
    vTaskDelay(2 * portTICK_PERIOD_MS);

   }
static void adc1_get_data_task(void *pvParameters)
{    

    int read_raw1, read_raw2, read_raw3;
    
    while(1) {  
     
       vTaskDelay(10 / portTICK_PERIOD_MS);
        read_raw1 = adc1_get_raw( ADC1_CHANNEL_6);//压电
        read_raw2 = adc1_get_raw( ADC1_CHANNEL_7);//压阻
        read_raw3 = adc1_get_raw( ADC1_CHANNEL_5);//VREF
        
        num1=num1+read_raw1;
        num2=num2+read_raw2;
        num3=num3+read_raw3;
        time1++;
        if (time1==10) {//发送原始数据相关代码
            time1=0;
            read_raw1=num1/10;
            read_raw2=num2/10;
            read_raw3=num3/10;
            num1=0;num2=0;num3=0;
            read_raw1=read_raw1-read_raw3;
           HTTP_Send_Data[http_send_flag*2]=read_raw1;
           HTTP_Send_Data[http_send_flag*2+1]=read_raw2;
            if(read_raw2<2000) low_power_flag++; else low_power_flag=0; //低功耗相关
           // printf("PV:  %d      PR:  %d   vref : %d   \n",  read_raw1,read_raw2,read_raw3);
           http_send_flag++;
            if(http_send_flag==10)
            {        time_t now;
                     struct tm timeinfo;
                     time(&now);
                     localtime_r(&now, &timeinfo);
                     if (timeinfo.tm_year < (2016 - 1900)) {
                     ESP_LOGI(TAGTIME, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
                     obtain_time();
                     time(&now);
                      }
                    char strftime_buf[64];
                    setenv("TZ", "CST-8", 1);
                    tzset();
                    localtime_r(&now, &timeinfo);
                    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
                    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);
                    cname=strftime_buf;
                    http_data_type_flag=1;
                    xSemaphoreGive(adc_semaphore);

                     http_send_flag=0;
            }
         
        }

          http_send_heart_flag++;
          if(http_send_heart_flag>=200)//十秒发送一次呼吸心率等信息
          {   vTaskDelay(10 / portTICK_PERIOD_MS);
              http_data_type_flag=2;
              HTTP_Body_Data[1]= HTTP_Body_Data[1]+1;
              if(HTTP_Body_Data[1]==30)HTTP_Body_Data[1]=20;
              HTTP_Body_Data[2]=50;
              HTTP_Body_Data[3]=36;
              printf("123:  %d      123:  %d   123 : %d   \n",  HTTP_Body_Data[1],HTTP_Body_Data[2],HTTP_Body_Data[3]);
                xSemaphoreGive(adc_semaphore);
                
                 http_send_heart_flag=0;
            }


           

            if(low_power_flag>=SAVE_POWER_WAIT_TIME)//如果满足条件，就进入低功耗
            {low_power_flag=0;
             vTaskResume(low_power_Handler);
            }else  vTaskSuspend(low_power_Handler);

          
    }
}
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
static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
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
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);  }
         /****************清除nvs_flash*************/
        ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
         ESP_ERROR_CHECK(  nvs_erase_key(handle,DATA1));
         usleep(5000*1000);
         ESP_ERROR_CHECK( nvs_erase_all(handle));
         usleep(5000*1000);
         ESP_ERROR_CHECK( nvs_commit(handle) );
         nvs_close(handle);
         esp_restart();
        ESP_LOGI(TAG,"connect to the AP fail");
        }else{//断线重连  
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);//清除标志位
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
         /****************清除nvs_flash*************/
        /* ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
         ESP_ERROR_CHECK(  nvs_erase_key(handle,DATA1));
         ESP_ERROR_CHECK( nvs_erase_all(handle));
         ESP_ERROR_CHECK( nvs_commit(handle) );
         nvs_close(handle);*/
       
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
        if(sntp_init_flag==0){sntp_init_flag=1;
/**************************wifi链接成功后，进行一次sntp时间戳初始化************************************/
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
     if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAGTIME, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
         ESP_LOGI(TAGTIME, "Time is not set yet. 123456");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    char strftime_buf[64];
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);}
/**************************wsntp时间戳初始化结束************************************/
    } else if (bits & WIFI_FAIL_BIT) {

    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");     
    }
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
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
            esp_smartconfig_stop();
            esp_restart();
            vTaskDelete(NULL);     
        }
    }
}
/******************smartconfig   over************************************************/
void app_main(void)
{
     uint32_t retry =0;
     time_t now=0;
     time_t return_now;
     struct tm timeinfo;
     char strftime_buf[64];
     timeinfo.tm_year = 0;
     esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
     ESP_ERROR_CHECK(ret);
      conn_mgr_init();
      initialise_wifi();//wifi链接
      IOT_SetLogLevel(IOT_LOG_INFO);
      // conn_mgr_start();
      adc_init();
     xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, NULL);
     xTaskCreate((void (*)(void *))ota_main, "ota_example", 20480, NULL, 9, NULL);
     xTaskCreate(&low_vol_protect_task, "low_vol_protect_task", 2048, NULL, 6,&low_power_Handler);  
     xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 7, NULL);
    adc_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!adc_semaphore) {
        ESP_LOGE(TAG, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
    }

     original_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!original_semaphore) {
        ESP_LOGE(TAG, "%s, init fail, the adc semaphore create fail.", __func__);
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
