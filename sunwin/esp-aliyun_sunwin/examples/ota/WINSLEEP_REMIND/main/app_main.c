#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
/*************延时函数相关******************/
#include <sys/time.h>
/*************RTOS相关******************/
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
/**********设备配网相关*************/
#include "esp_smartconfig.h"
/**********LWIP协议栈相关*************/
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
/**********在线ota*************/
#include "esp_ota_ops.h"
#include <sys/socket.h>
#include "infra_compat.h"
#include "ota_solo.h"
#include "conn_mgr.h"
/***********算法相关***********/
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"
#include "sw_al.h"
/********低功耗相关头文件******/
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
#include "sdkconfig.h"
#define SEND_TIMES 3
#define DATA_NUM 20
#define  SAVE_POWER_WAIT_TIME    300//5分钟
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
#define LED_G_IO 		21
#define false		0
#define true		1
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define start_time 8
#define stop_time 20
#define LEFT_TIME_OF_OUT_BED 1
static RTC_DATA_ATTR struct timeval sleep_enter_time;
static SemaphoreHandle_t adc_semaphore,original_semaphore,led_semaphore;
//wifi函数
static EventGroupHandle_t  wifi_event_group,s_wifi_event_group;
static int s_retry_num = 0;
static const int CONNECTED_BIT = BIT0,ESPTOUCH_DONE_BIT = BIT1;
static const char *DATA1 = "nvs_blob",*NVS_CUSTOMER = "customer data",*TAGTIME = "TIME",*TAG = "HTTP_CLIENT",*TAG1 = "smartconfig_example",*TAG2 = "wifi station";
char *cname,*month,post_data2[2048]={0},wifi_flag=0,http_send_fail=0,left_bed_flag=0,bed_touch_flag=0,move_body_flag=0;
short sleep_post_flag=0,http_send_flag=0,low_power_flag=0,sntp_init_flag=0,http_data_type_flag=0;
int HTTP_Send_Data[20]={0},HTTP_Body_Data[6]={7,20,0,0,0,1},HTTP_MonitorSleep_Data[1]={3},remind=0;
int lengthtime=0,current_hour=0,current_min=0;
extern const char *Device_ID;
extern char Ota_start_flag;
const char *address,*webaddress[5]={"http://up.winsleep.club/originalMonitor.action","http://up.winsleep.club/monitor.action","http://up.winsleep.club/monitorSleep.action","http://up.winsleep.club/abnormalAndService.action","http://up.winsleep.club/remind.action"};
TaskHandle_t led_test_task_Handler,http_test_task_Handler,low_power_Handler,adc1_get_data_task_Handler,over_time_scan_task_Handler;
nvs_handle handle;
//http_data_type_flag=1  表示原始数据
//http_data_type_flag=2  表示呼吸心率信息
//http_data_type_flag=3  表示睡眠状态信息
//http_data_type_flag=4  表示长期离床报警信息
static void low_vol_protect_task(void *arg);
static void over_time_scan_task(void *pvParameters);
static void wifi_led_test_task(void *pvParameters);//wifi断开网络状态指示灯
static void led_test_task(void *pvParameters);
static void adc1_get_data_task(void *pvParameters);
static void smartconfig_example_task(void *parm);
 static void sntp_task(void *pvParameters);
static void obtain_time(void);
static void adc_init(void);
void make_further_polishment( double absmax,
                                         double *p_resp_value, double *p_heart_value,
                                            int *p_resp_exception_counter, int *p_heart_exception_counter,
                                            history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos);
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");
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
             //ESP_LOGI(TAG, "Receivedata=  %s            \n", (char*)evt->data);
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
    char post_data1[2048]={0};
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
            http_send_fail=0;
            memset(post_data1, 0,strlen(post_data1));
            memset(post_data2, 0,strlen(post_data2));
        } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        xTaskCreate(&wifi_led_test_task, "led_test_task", 2048, NULL, 11, NULL);//数据发送不成功 ，灯快速闪动
        http_send_fail++;
        if(http_send_fail==5){  
            http_send_fail=0;
            esp_restart();
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
                    {   printf("原始数据上传一次.............\n");
                        address = webaddress[0];
                        printf("address1:%s \n",address);
                        Get_Current_Original_Data(post_data2);//得到json数据，用如post data
                        http_data_type_flag=0;
                    }
                    if(http_data_type_flag==2){
                        printf("体征数据上传一次..................\n");
                        address = webaddress[1];
                        printf("address2:%s \n",address);
                        Get_Current_Body_Data(post_data2);//得到json数据，用如post data
                        http_data_type_flag=0;
                        }
                    if(http_data_type_flag==3){
                        printf("睡眠信息上传一次..................\n");          
                        address = webaddress[2];
                        printf("address3:%s \n",address);
                        Get_Current_monitorSleep_Data(post_data2);//得到json数据，用如post data
                        http_data_type_flag=0;
                        }
                     if(http_data_type_flag==5){
                        printf("离床报警标志上传一次..................\n");
                        address = webaddress[4];
                        printf("address5:%s \n",address);
                        Get_Current_remind_Data(post_data2);//得到json数据，用如post data
                        printf("http_data_type_flag:%d   post_data2:%s\n",http_data_type_flag,post_data2); 
                        http_data_type_flag=0;
                        }
                        http_rest_with_url();
    }
    }
    }
    vTaskDelete(NULL);
}

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{  nvs_handle handle;
    esp_err_t err;
    static const char *NVS_CUSTOMER = "customer data";
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
            }else if(err == ESP_OK)
            {wifi_flag=1;
            nvs_close(handle);
            esp_wifi_connect();
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
        }
        else{//断线重连  密码错误尝试重新链接
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);//清除标志位
        ESP_LOGI(TAG, "断线重连.........");
        esp_restart();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {//获取到wifi信息后，执行， 第七步
        if (wifi_flag == 1){  
         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
         ESP_LOGI(TAG2, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
         s_retry_num = 0;
         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      }else xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {//等待配网 第四步，等待手机配网
        ESP_LOGI(TAG1, "Scan done");//睡眠分期数据上传标志、如何本次上传后，则后续if语句不成立
        xTaskCreate(&led_test_task, "led_test_task", 2048, NULL, 10, &led_test_task_Handler); 
        xTaskCreate(&over_time_scan_task, "over_time_scan_task", 2048, NULL, 11, &over_time_scan_task_Handler); 
        printf("Creat over_time_scan_task!!!!!\n ");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {//扫描信道 第五步，找到udp广播
        ESP_LOGI(TAG1, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) { //获取到ssid和密码   第六步，获取 wifi信息
        ESP_LOGI(TAG1, "Got SSID and password");
        vTaskDelete(over_time_scan_task_Handler);
        printf("Delete over_time_scan_task!!!!!!\n ");
        vTaskSuspend(led_test_task_Handler);
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) 
         memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
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
        ESP_ERROR_CHECK( esp_wifi_connect() );
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}
/*初始化 wifi*/
static void initialise_wifi(void)
{   wifi_event_group = xEventGroupCreate();//创建一个事件组
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
    static const char *DATA1 = "nvs_blob";
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
    if (timeinfo.tm_year < (2016 - 1900)) {
     ESP_LOGI(TAGTIME, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
     ESP_LOGI(TAGTIME, "Time is not set yet. ");
     obtain_time();
     time(&now);
    }
    char strftime_buf[64];
    setenv("TZ", "CST-8", 1);
    tzset();
    struct tm *tm=localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);
    }
/**************************wsntp时间戳初始化结束************************************/
    } else if (bits & WIFI_FAIL_BIT) {} 
    else ESP_LOGE(TAG, "UNEXPECTED EVENT");
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
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
    adc_semaphore = xSemaphoreCreateBinary();
    if (!adc_semaphore) {
     ESP_LOGE(TAG, "%s, init fail, the adc semaphore create fail.", __func__);
     return;
    }    
    led_semaphore = xSemaphoreCreateBinary();
    if (!led_semaphore) {
     ESP_LOGE(TAG, "%s, init fail, the adc semaphore create fail.", __func__);
     return;
    }
    original_semaphore = xSemaphoreCreateBinary();
    if (!original_semaphore) {
        ESP_LOGE(TAG, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
     }
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
     }
    ESP_ERROR_CHECK(ret);
    gpio_pad_select_gpio(LED_G_IO);
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_G_IO, 0);
    conn_mgr_init();
    adc_init();//adc初始化
    initialise_wifi();//wifi链接
    IOT_SetLogLevel(IOT_LOG_INFO);//aliyun模型初始化
    xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, &adc1_get_data_task_Handler);
    xTaskCreate((void (*)(void *))ota_main, "ota_example", 20480, NULL, 6, NULL);
    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 7, &http_test_task_Handler);//高优先级通过获取二值信号量
  //  xTaskCreate(&sntp_task, "sntp_task", 2048, NULL, 8,NULL);  //创建低功耗任务
}
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
static void obtain_time(void)
{
    ESP_LOGI(TAGTIME, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "cn.ntp.org.cn");
    sntp_setservername(1, "210.72.145.44");		// 国家授时中心服务器 IP 地址
    sntp_setservername(2, "1.cn.pool.ntp.org");        
    sntp_init();
    setenv("TZ", "CST-8", 1);//设置时区
    tzset();//设置时区
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(TAGTIME, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    if(retry>=9)esp_restart();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);//获取网络时间，64bit的秒计数
    localtime_r(&now, &timeinfo);//转化为具体的时间参数
}
static void adc1_get_data_task(void *pvParameters)
{    
    time_t now;
    struct tm timeinfo;
    int output_data=0;
    uint32_t num1=0,num2=0,num3=0;
    uint8_t time1=0;
    int read_raw1, read_raw2, read_raw3;
    int read_raw11, read_raw22, read_raw33,read_raw111;
    uint64_t data_index = 0;
    int begin1 = 0;
    int begin2 = 0;
    int begin3 = 0;
    int Fs = 10;
    int sample_points = 100/Fs;
    //每段信号100个数据点
    int N_seg = 100;
	int N_seg_1 = 75;
    //心率每段信号100个数据点
    int N_heart = 1000;
     int N_heart_1 = 750;
    //重迭点数
    double seg_energy = 0.;
    double seg_pre_abssum = 0.;
    double resp_value = 0.;
    double heart_value = 0.;
    double pr_diff = 0.;
    double absmax = 0.;
    double std_move=0;
    int people_counter = 6; //重物判断参数
    int not_people_counter = 0;//重物判断参数
    //每次取100个数据点进行检测
    double * X_X = (double *)malloc(N_seg*sizeof(double));
    double * Pr_X = (double *)malloc(N_seg*sizeof(double));
	 	//用于存放前面重叠的点
    double * X_X_1 = (double *)malloc(N_seg_1*sizeof(double));
    double * Pr_X_1 = (double *)malloc(N_seg_1*sizeof(double));
   		//用于存放后面叠加的点
    double * X_X_2 = (double *)malloc((N_seg - N_seg_1)*sizeof(double));
    double * Pr_X_2 = (double *)malloc((N_seg - N_seg_1)*sizeof(double));
    //心率1000个数据点
    double * X_X_heart = (double *)malloc(N_heart*sizeof(double));
    double * X_X_heart_1 = (double *)malloc(N_heart_1*sizeof(double));
    double * X_X_heart_2 = (double *)malloc((N_heart - N_heart_1)*sizeof(double));
    // 0 正常 1 低通气 2 睡眠暂停 3 呼吸过快 4 坠床 5 体动
	double * resp_1_minute = (double *)malloc(24*sizeof(double));
	double * heart_1_minute = (double *)malloc(24*sizeof(double));
    double * update_heart_10_minute = (double *)malloc(240*sizeof(double));
    double * std_array_t=(double *)malloc(72*sizeof(double));
    double *mean_heart_array=(double *)malloc(240*sizeof(double));
    uint64_t count_time = 0;
    int std_t=0;
	int count_t = 0;
	int count_stage = 0,mean_heart_t=0,rem_leave_t=0,rem_move_t=0;
    double event_type_vec;
    int log_level = 0;
	uint8_t hist_stat_arr_size = 2;
	history_statistics hist_stat_arr[hist_stat_arr_size];
	int hist_stat_arr_rear = 0;
	history_statistics cur_stat;
	memset(hist_stat_arr,0x00,sizeof(hist_stat_arr));
	memset(&cur_stat,0x00,sizeof(cur_stat));
	uint64_t datanum_after_inbed = 0;
	uint8_t enable_further_judege = 1;
	int i_resp_exception_counter = 0;
	int i_heart_exception_counter = 0;
	double	max_value_index[2] = {0.,0.};
	enum mon_period { IDLE = 0, IN_BED, AFTER_ADAPT };
	uint8_t current_period = IDLE;
	int out_num = 0;
    gpio_set_level(LED_G_IO, 0);
    while(1) {  
        event_type_vec = 0;
        vTaskDelay(10/ portTICK_PERIOD_MS);
        time1++;
        read_raw1 = adc1_get_raw( ADC1_CHANNEL_6);//压电
        read_raw2 = adc1_get_raw( ADC1_CHANNEL_7);//压阻
        read_raw3 = adc1_get_raw( ADC1_CHANNEL_5);//VREF
        num1=num1+read_raw1;
        num2=num2+read_raw2;
        num3=num3+read_raw3;
        read_raw11 = read_raw1;
        read_raw22 = read_raw2;
        read_raw33 = read_raw3;
/********************************算法相关*********************/
        if (time1 % sample_points == 0) {
            output_data++;
            read_raw1=num1/sample_points;
            read_raw2=num2/sample_points;
            read_raw3=num3/sample_points;
            num1=0;num2=0;num3=0;
           read_raw111=read_raw1-read_raw3;
           HTTP_Send_Data[http_send_flag*2]=read_raw111;
           HTTP_Send_Data[http_send_flag*2+1]=read_raw2;
           http_send_flag++;
           if(http_send_flag==10) {//采集了100组数据
            printf("%llu      %d      %d      %d  \n", data_index, read_raw1,read_raw2, read_raw3 );
            if(Ota_start_flag==1){//每秒检验一次是否有升级命令
             Ota_start_flag=0;
             vTaskSuspend(http_test_task_Handler );
             vTaskSuspend(adc1_get_data_task_Handler );
             }   
            time(&now);
            char strftime_buf[64];
            char strftime_month_buf[64];
            struct tm *tm= localtime_r(&now, &timeinfo);
            strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
            strftime(strftime_month_buf, sizeof(strftime_month_buf), "%Y-%m", &timeinfo);//组建时间戳
            cname=strftime_buf;
            month=strftime_month_buf;
            current_hour = tm->tm_hour;//获取当前小时
            current_min =tm->tm_min;//获取当前分钟
            http_send_flag=0;//标志位清零
            if(read_raw2<2000) low_power_flag++; else low_power_flag=0; //低功耗相关
            if(low_power_flag>=SAVE_POWER_WAIT_TIME){//如果满足条件，就进入低功耗
             low_power_flag=0;//标志位清零
             xTaskCreate(&low_vol_protect_task, "low_vol_protect_task", 2048, NULL, 6,&low_power_Handler);  //创建低功耗任务
            }
            http_data_type_flag=1;
            xSemaphoreGive(adc_semaphore);//满足条件时，上传原始数据,释放二值信号量，发送原始数据
            vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
            }
            if(data_index == 0) {
             if(read_raw2 < 1000) 
              output_data = 0;
             if(output_data < 10){
              begin3 = 0;
              continue;}
           }
            data_index++;}
            X_X_heart[begin3++] = read_raw11 - read_raw33;//每次都先取一些数据放在这个数组里面
            if(time1 % sample_points != 0)
             continue;//取原始数据，取100次，跳出本次循环100次
            time1 = 0;//取完成以后，标志位清零
            //基准数组
            current_period = IN_BED;
            if(data_index <= N_seg){
                X_X[begin1] = read_raw1 - read_raw3;
                Pr_X[begin1] = read_raw2;
                begin1++;
                if(begin1==N_seg){
                    begin1 = 0;
                    begin3 = 0;}
            }
		//将上一个75点的呼吸率数据备份
        Data_transfer(X_X_1, X_X ,N_seg - N_seg_1, N_seg);
        Data_transfer(Pr_X_1, Pr_X ,N_seg - N_seg_1, N_seg);
        //将上一个750点的呼吸率心率数据备份
        Data_transfer(X_X_heart_1, X_X_heart , N_heart - N_heart_1, N_heart);
        if(data_index > N_seg){
            current_period = AFTER_ADAPT; 
            X_X_2[begin2] = read_raw1 - read_raw3; //压电取一百个原始数据，当大于100个数据时，开始进行计算
            Pr_X_2[begin2] = read_raw2; //压阻
            begin2++;
            if(begin2==N_seg - N_seg_1){//如果取了 2.5的数据
            count_time++;
            if(count_time==1){//开机上传一次清醒标志
                HTTP_MonitorSleep_Data[0]=3;//进入清醒
                printf("WOKE\n");
                http_data_type_flag=3;
                xSemaphoreGive(adc_semaphore);
                vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
            }
            clock_t start, finish;//计算算法的时间
            double  duration;
            start = clock();
            begin2 = 0;
            begin3 = 0;
            Data_transfer(X_X_heart_2, X_X_heart, 0,N_heart - N_heart_1);
			//拼凑前7.5秒和当前2.5秒的数据
			Data_transfer(X_X,X_X_1,0,N_seg_1);
			Data_transfer(Pr_X,Pr_X_1,0,N_seg_1);
			Data_transfer(X_X_heart,X_X_heart_1,0,N_heart_1);
			Data_transfer(X_X+N_seg_1,X_X_2,0,N_seg - N_seg_1);
			Data_transfer(Pr_X+N_seg_1,Pr_X_2,0,N_seg - N_seg_1);
			Data_transfer(X_X_heart+N_heart_1,X_X_heart_2,0,N_heart - N_heart_1);
            double * out_sign = (double *)malloc(143*sizeof(double));
            int * out_levels = (int *)malloc(5*sizeof(int));
            double * a2_resp = (double *)malloc(N_seg*sizeof(double));
            //double * a2_heart = (double *)malloc(N_seg*sizeof(double));
            wavedec(X_X, out_sign, out_levels, N_seg, 5);
            wrcoef(out_sign,out_levels,a2_resp);
            free(out_sign);out_sign = NULL;
            free(out_levels);out_levels = NULL;
             //计算压阻绝对值之和
            seg_pre_abssum = onearray_sum_fabs(Pr_X,N_seg);
             //计算能量值
            seg_energy = accumulate1(a2_resp,N_seg,N_seg);
            if(seg_pre_abssum < 500){     
                printf("has left bed\n");
                datanum_after_inbed = 0;
                bed_touch_flag = 0;//清空在床标志位
                free(a2_resp);a2_resp = NULL;
				finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC;
                HTTP_Body_Data[1]=0;
                HTTP_Body_Data[3]=0;
                HTTP_Body_Data[0]=7;
                left_bed_flag++;
                if(left_bed_flag>=SEND_TIMES)left_bed_flag=SEND_TIMES;
                if(left_bed_flag<SEND_TIMES){      
                    rem_leave_t++;//五分钟内是否出现离床，用于睡眠分期使用
                    http_data_type_flag=2;
                    xSemaphoreGive(adc_semaphore);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                    remind=1;
                    http_data_type_flag=5;//remind接口内部发送离床告示
                    xSemaphoreGive(adc_semaphore);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                 }
                if(count_time % 24 == 0){
                 count_t = 0;
				 if(count_time % 240 == 0);
                }
                	continue;
             }
            if(judge_whether_human(X_X,N_seg) == 1){
                resp_value = 0.;
                heart_value = 0.;
                not_people_counter = not_people_counter + 1;
				if(not_people_counter>=6) people_counter = 0; }
            else{
                not_people_counter = 0;
                people_counter =  people_counter +1;
            }
            if(not_people_counter >= 6 || people_counter <= 5 && HTTP_Body_Data[0]==8) {
                printf("it is not human\n");
                vTaskDelay(250 / portTICK_PERIOD_MS);//预留时间给http任务
                datanum_after_inbed = 0;
                bed_touch_flag = 0;
                 //清理之前分配的内存
                free(a2_resp);a2_resp = NULL;
                 //free(a2_heart);a2_heart = NULL;
                finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC;
                HTTP_Body_Data[1]=0;
                HTTP_Body_Data[3]=0;
                HTTP_Body_Data[0]=1;
                left_bed_flag++;
                if(left_bed_flag>=SEND_TIMES)left_bed_flag=SEND_TIMES;
                if(left_bed_flag<SEND_TIMES){
                        http_data_type_flag=2;
                        xSemaphoreGive(adc_semaphore);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                        remind=1;
                        http_data_type_flag=5;//remind接口内部发送离床告示
                        xSemaphoreGive(adc_semaphore);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务             
                }
                if(count_time % 24 == 0){ 
                    count_t = 0;
                    if(count_time % 240 == 0);
                    }
                    continue;
                }
				double * out_diff = (double *)malloc(N_seg*sizeof(double));
				diff_fabs(Pr_X,N_seg,out_diff,&out_num);//检验数据波动性
				pr_diff = onearray_sum(out_diff,out_num);//求累计和
				free(out_diff);out_diff = NULL;
                std_move= standard_deviation(Pr_X,0,N_seg);//每间隔十分钟，计算一次标准差，并将标准差备份
				if(pr_diff >= 2000){
					printf("there is body movement\n");
                    vTaskDelay(250 / portTICK_PERIOD_MS);//预留时间给http任务
                    datanum_after_inbed = 0;
					free(a2_resp);a2_resp = NULL;
					finish = clock();
					duration = (double)(finish - start) / CLOCKS_PER_SEC;
                    HTTP_Body_Data[1]=0;
                    HTTP_Body_Data[3]=0;
                    HTTP_Body_Data[0]=8;
                    left_bed_flag=0;//离床标志位清零
                    move_body_flag++;
                    if(move_body_flag>=SEND_TIMES)move_body_flag=SEND_TIMES;
                    if(move_body_flag<SEND_TIMES){     
                        rem_move_t++;
                        http_data_type_flag=2;
                        xSemaphoreGive(adc_semaphore);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                    }
                    bed_touch_flag++;
                    if(bed_touch_flag>=SEND_TIMES)bed_touch_flag=SEND_TIMES;
                    if(bed_touch_flag<SEND_TIMES){      
                            remind=0;
                            http_data_type_flag=5;//remind接口内部发送离床告示
                            xSemaphoreGive(adc_semaphore);
                            vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                    }

                    if(count_time % 24 == 0){
                     count_t = 0;
					 if(count_time % 240 == 0)
                      ;
                    }
					    continue;
				}   
                //计算呼吸频率
                resp_value = obtain_resprate_with_max_possibility(a2_resp,N_seg);
                //计算心率
                heart_value = obtain_heart_by_spec(X_X_heart,N_heart);
                //计算压电信号最大值
                max_v_interval_fabs(X_X,max_value_index,N_seg);
                absmax = max_value_index[0];
                cur_stat.absmax = absmax;
                cur_stat.seg_energy = seg_energy;
                cur_stat.pr_diff = pr_diff;
                cur_stat.resp_value = resp_value;
                cur_stat.heart_value = heart_value;
                if(datanum_after_inbed == 0) {
                    save_history_info(hist_stat_arr, hist_stat_arr_size,&hist_stat_arr_rear, cur_stat); //do nothing
                }
                else if (enable_further_judege == 1){//根据过往信息进一步判断
                uint8_t hist_stat_arr_front = 0;
                if (datanum_after_inbed >= hist_stat_arr_size) //历史数组中已经有两个数据了
                  hist_stat_arr_front = 1;
                	//结合上两个10秒，进一步判断结果
                  make_further_polishment(absmax,
                                                 &resp_value, &heart_value,
                                                 &i_resp_exception_counter, &i_heart_exception_counter,
                                                 hist_stat_arr,hist_stat_arr_size, hist_stat_arr_front);
                  cur_stat.resp_value = resp_value;
                  cur_stat.heart_value = heart_value;
                  save_history_info(hist_stat_arr, hist_stat_arr_size,&hist_stat_arr_rear, cur_stat);
                }
                else
                ; //To do
                datanum_after_inbed++;
                //清理加输出
                free(a2_resp);a2_resp = NULL;
                printf("resp_value = %f,heart_value = %f\n\n",resp_value,heart_value);
                vTaskDelay(250 / portTICK_PERIOD_MS);//预留时间给http任务
                 if((resp_value<0)||(heart_value<0))
                    ;
               else{
                HTTP_Body_Data[1]=(int)resp_value;
                HTTP_Body_Data[3]=(int)heart_value;
                HTTP_Body_Data[0]=6;}
                left_bed_flag=0;//清除离床标志
                move_body_flag=0;//清除体动标志
                http_data_type_flag=2;
                xSemaphoreGive(adc_semaphore);
                vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                bed_touch_flag++;
                if(bed_touch_flag>=SEND_TIMES)bed_touch_flag=SEND_TIMES;
                if(bed_touch_flag<SEND_TIMES){      
                        remind=0;
                        http_data_type_flag=5;//remind接口内部发送离床告示
                        xSemaphoreGive(adc_semaphore);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);//预留时间给http任务
                 }
                resp_1_minute[count_t] = resp_value;
				heart_1_minute[count_t] = heart_value;
                count_t++;
				if(count_stage<240){//采集十分钟的心率以及呼吸值
					update_heart_10_minute[count_stage] = heart_value;
                    count_stage++;
				}    
				if(count_time%24 == 0){
					double mean_resp = mean(resp_1_minute,0,count_t);
					double mean_heart = mean(heart_1_minute,0,count_t);
					printf("1 minute average resp_value = %f,1 minute average heart_value = %f\n",mean_resp,mean_heart);
                    HTTP_Body_Data[2]=(int)mean_resp;
                    HTTP_Body_Data[4]=(int)mean_heart;
					count_t = 0;
				}
                if(count_stage >=240){
                        count_stage=0;
						std_array_t[std_t] = standard_deviation(update_heart_10_minute,0,240);//每间隔十分钟，计算一次标准差，并将标准差备份
                        mean_heart_array[std_t]=mean(update_heart_10_minute,0,240);
                        if(std_t>=1){
                                if(( HTTP_MonitorSleep_Data[0]==2)||(HTTP_MonitorSleep_Data[0]==1)){
                                    if((mean_heart_array[std_t]>=mean_heart_array[std_t-1])&&(sleep_post_flag==0)){
                                                    rem_leave_t=0;
                                                    rem_move_t=0;
                                                    HTTP_MonitorSleep_Data[0]=2;//进入浅睡眠
                                                    printf("REM\n");
                                                    http_data_type_flag=3;
                                                    xSemaphoreGive(adc_semaphore);
                                                    vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                                                    sleep_post_flag=1;//睡眠分期数据上传标志、如何本次上传后，则后续if语句不成立
                                                }
                                   }
                                if((std_array_t[std_t]<=std_array_t[std_t-1])&&(sleep_post_flag==0)){//与前一次标准差做比对  
                                    rem_leave_t=0;
                                    rem_move_t=0;
                                    HTTP_MonitorSleep_Data[0]=1;//进入深睡眠
                                    printf("NREM\n");
                                    http_data_type_flag=3;
                                    xSemaphoreGive(adc_semaphore);
                                    vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                                     sleep_post_flag=1;//睡眠分期数据上传标志、如何本次上传后，则后续if语句不成立
                                }
                                if((std_array_t[std_t]>std_array_t[std_t-1])&&(sleep_post_flag==0)){//与前一次标准差做比对
                                    rem_leave_t=0;
                                    rem_move_t=0;
                                    HTTP_MonitorSleep_Data[0]=3;//进入清醒
                                    printf("WOKE\n");
                                    http_data_type_flag=3;
                                    xSemaphoreGive(adc_semaphore);
                                    vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                                     sleep_post_flag=1;//睡眠分期数据上传标志、如何本次上传后，则后续if语句不成立
                                }
                        }
                        	std_t++;
                            sleep_post_flag=0;
					}
                    finish = clock();
                    duration = (double)(finish - start) / CLOCKS_PER_SEC;
                    printf( "\n accumulate time:%f seconds\n", duration );
            }        
        }
    }
    free(X_X); X_X = NULL;
    free(Pr_X); Pr_X = NULL;
	free(X_X_heart); X_X_heart = NULL;
    free(X_X_1); X_X_1 = NULL;
    free(Pr_X_1); Pr_X_1 = NULL;
    free(X_X_2); X_X_2 = NULL;
    free(Pr_X_2); Pr_X_2 = NULL;
    free(X_X_heart_1); X_X_heart_1 = NULL;
    free(X_X_heart_2); X_X_heart_2 = NULL;
    free(resp_1_minute);resp_1_minute=NULL;
    free(heart_1_minute);heart_1_minute=NULL;
}
   static void led_test_task(void *pvParameters)
{
    while(1){
            gpio_set_level(LED_G_IO, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_G_IO, 0);  
            vTaskDelay(1000 / portTICK_PERIOD_MS);///freertos里面的延时函数
    }
    vTaskDelete(NULL);  
}

   static void over_time_scan_task(void *pvParameters)
{  
    int scan_time=0;
    while(1){ 
                            if(scan_time>12){//如果扫描时长超过两分钟
                                xTaskCreate(&low_vol_protect_task, "low_vol_protect_task", 2048, NULL, 6,&low_power_Handler);  //创建低功耗任务
                            }
                            for(int i=0;i<10;i++)vTaskDelay(1000/portTICK_PERIOD_MS);///freertos里面的延时函数
                            scan_time++;
                      }
    vTaskDelete(NULL);  
}
   static void sntp_task(void *pvParameters)
{  
    while (1){
    for(int i=0;i<100;i++)vTaskDelay(1000/portTICK_PERIOD_MS);///freertos里面的延时函数
   /**************************wifi链接成功后，进行一次sntp时间戳初始化************************************/

/**************************wsntp时间戳初始化结束************************************/
    }
    vTaskDelete(NULL);  
}
static void wifi_led_test_task(void *pvParameters)//wifi断开
{
    for(int i=0;i<3;i++){  
            gpio_set_level(LED_G_IO, 1);
            vTaskDelay(50 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_G_IO, 0);  
            vTaskDelay(50 / portTICK_PERIOD_MS);///freertos里面的延时函数
          }
             vTaskDelete(NULL);
}

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
        {      ESP_LOGI(TAG1, "smartconfig over");
               esp_smartconfig_stop();

/**************************wifi链接成功后，进行一次sntp时间戳初始化************************************/
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
     ESP_LOGI(TAGTIME, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
     ESP_LOGI(TAGTIME, "Time is not set yet. ");
     obtain_time();
     time(&now);
    }
    char strftime_buf[64];
    setenv("TZ", "CST-8", 1);
    tzset();
    struct tm *tm=localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);
/**************************wifi链接成功后，进行一次sntp时间戳初始化************************************/
           
               ESP_LOGI(TAG, "smartconfig 里面的任务");
               vTaskDelete(led_test_task_Handler);
               xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, &adc1_get_data_task_Handler);
               xTaskCreate((void (*)(void *))ota_main, "ota_example", 20480, NULL, 6, NULL);
               xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 7, &http_test_task_Handler);
               //xTaskCreate(&sntp_task, "sntp_task", 2048, NULL, 8,NULL);  //创建低功耗任务
               vTaskDelete(NULL);     
        }
    }
}
void make_further_polishment(double absmax,
                             double *p_resp_value, double *p_heart_value,
                                int *p_resp_exception_counter, int *p_heart_exception_counter,
                                history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos) {
	double history_resp_sum = 0.;
	double history_heart_sum = 0.;
	uint8_t field = -1;
		 //std函数比较费时间，暂时不用
     uint8_t whether_resp_abnormal = (*p_resp_value > 35 || *p_resp_value < 10);
     if(whether_resp_abnormal == 1) {//异常的呼吸率
                *p_resp_exception_counter = *p_resp_exception_counter + 1;
                if(*p_resp_exception_counter < 3) {
                		field = 3;
                   	history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
                		*p_resp_value = history_resp_sum/(front_pos - 0 + 1);
                }
                else
                {
                    ;//todo = 0;%不改变当前计算值,报告出去
                }
      }
     if(whether_resp_abnormal == 0) { //正常的呼吸率,用当前和过往的做平均
     				field = 3;
            history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
            *p_resp_value = (history_resp_sum + *p_resp_value)/((front_pos - 0 + 1) + 1);
            *p_resp_exception_counter = 0;
     }
     //======================心率================================ [
     uint8_t whether_heart_abnormal = (*p_heart_value > 150 || *p_heart_value < 50);
     if(whether_heart_abnormal == 1 ) //异常的心率
     {
     			*p_heart_exception_counter = *p_heart_exception_counter + 1;
          if(*p_heart_exception_counter < 3) {
          		field = 4;
             	history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
          		*p_heart_value = history_heart_sum/(front_pos - 0 + 1);
          }
          else
          {
              ;//todo = 0;%不改变当前计算值,报告出去
          }
//     		}
     }
     if(whether_heart_abnormal == 0) //正常的心率,用当前和过往的做平均
     {
         field = 4;
         history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
         *p_heart_value = (history_heart_sum + *p_heart_value)/((front_pos - 0 + 1) + 1);
         *p_heart_exception_counter = 0;
     }
     // ========================================================= ]
}
