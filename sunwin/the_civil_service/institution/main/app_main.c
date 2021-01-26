/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "protocol_examples_common.h"
#include "esp_smartconfig.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include"cJSON.h"
/***********算法相关***********/
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"
/********低功耗相关头文件******/
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
#include "sdkconfig.h"
#include "esp_http_client.h"


static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t wifi_event_group;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

#define  SAVE_POWER_WAIT_TIME    300//5分钟
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
#define LED_G_IO 		21
#define false		0
#define true		1
#define MAX_HTTP_RECV_BUFFER 512
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define start_time 20
#define stop_time 8
#define LEFT_TIME_OF_OUT_BED 30
static RTC_DATA_ATTR struct timeval sleep_enter_time;
static SemaphoreHandle_t adc_semaphore,original_semaphore,led_semaphore,mqtt_semaphore;
//wifi函数
static int s_retry_num = 0;
static const char *DATA1 = "nvs_blob",*NVS_CUSTOMER = "customer data",*TAGTIME = "TIME",*TAG3 = "HTTP_CLIENT",*TAG1 = "smartconfig_example",*TAG2 = "wifi station",*TAG4 = "HTTP_CLIENT";
char *cname,*month,*waring_string="bed_off",*waring_type="bed_off",post_data1[1048],post_data2[1048]={0},wifi_flag=0,http_send_fail=0;
short sntp_init_flag=0,http_data_type_flag=0;
int  HTTP_Body_Data[6]={0,0,3,0,0,1},MQTT_Body_Data[6]={0,0,3,0,0,1};
int out_bed_flag=0,lengthtime=0,waring_flag=0,min_of_out_bed=0,current_hour=0,current_min=0;
//http_data_type_flag=1  报警信息
//http_data_type_flag=2  表示呼吸心率信息
const char *Device_ID="SWA002";
const char *address,*webaddress[3]={"http://up.winsleep.club/abnormalAndService.action","http://up.winsleep.club/monitor.action","http://up.winsleep.club/monitorSleep.action"};
TaskHandle_t led_test_task_Handler,http_test_task_Handler,low_power_Handler,adc1_get_data_task_Handler;
nvs_handle handle;
static void low_vol_protect_task(void *arg);
static void wifi_led_test_task(void *pvParameters);//wifi断开网络状态指示灯
static void led_test_task(void *pvParameters);
static void adc1_get_data_task(void *pvParameters);
static void smartconfig_example_task(void *parm);
static void obtain_time(void);
static void adc_init(void);
//static EventGroupHandle_t mqtt_event_group;
//const static int CONNECTED_BIT = BIT0;
static void Get_Current_Body_Data(char *dat);
static void Get_Current_Original_Data(char *dat);
static void Get_Current_monitorSleep_Data(char *dat);
static esp_mqtt_client_handle_t mqtt_client = NULL;
static const char *TAG_MQTT = "MQTT_EXAMPLE";
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
            ESP_LOGD(TAG4, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG4, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG4, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG4, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
           ESP_LOGD(TAG4, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
             //ESP_LOGI(TAG, "Receivedata=  %s            \n", (char*)evt->data);
             printf("%.*s   \n", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG4, "HTTP_EVENsT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG4, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG4, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG4, "Last mbedtls failure: 0x%x", mbedtls_err);
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
    esp_http_client_handle_t client = esp_http_client_init(&config);
    char post_data1[1024]={0};
    // POST
    memcpy(post_data1, post_data2,strlen(post_data2));
    esp_http_client_set_url(client,address);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data1, strlen(post_data1));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG4, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                  http_send_fail=0;
    } else {
        ESP_LOGE(TAG4, "HTTP POST request failed: %s", esp_err_to_name(err));
         xTaskCreate(&wifi_led_test_task, "led_test_task", 2048, NULL, 11, NULL);//数据发送不成功 ，灯快速闪动
        http_send_fail++;
        if(http_send_fail==20)
        {
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
                    {  printf("报警数据上传一次.............\n");
                        
                            address = webaddress[0];
                            printf("address1:%s \n",address);
                            Get_Current_monitorSleep_Data(post_data2);//得到json数据，用如post data
                            printf("http_data_type_flag:%d   post_data2:%s\r\n",http_data_type_flag,post_data2);
                            http_data_type_flag=0;
                    }
                        if(http_data_type_flag==2){
                            printf("体征数据上传一次..................\n");
                            address = webaddress[1];
                            printf("address2:%s \n",address);
                            Get_Current_Original_Data(post_data2);//得到json数据，用如post data
                            printf("http_data_type_flag:%d   post_data2:%s\n",http_data_type_flag,post_data2);
                            http_data_type_flag=0;
                        }
                            http_rest_with_url();
                    }
                    }
                    memset(post_data2,0,strlen(post_data2));
    }
    vTaskDelete(NULL);
}static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/products/mattress/SWA002", "HELLO,FRIEND", 0, 1, 0);
            ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

       //     msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
       //     ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

           // msg_id = esp_mqtt_client_subscribe(client, "/products/mattress/SWA002", 1);
           // ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

          //  msg_id = esp_mqtt_client_unsubscribe(client,"/products/mattress/SWA002");
         //   ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
        ESP_LOGI(TAG_MQTT, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
        esp_mqtt_client_start(mqtt_client);
}
static void mqtt_task(void *Pvparamters)
{
while(1){
    if(mqtt_semaphore!=NULL)
    {
                    int err = xSemaphoreTake(mqtt_semaphore, portMAX_DELAY);
                    if(err== pdTRUE){
                                Get_Current_Body_Data(post_data2);
                                int  msg_id = esp_mqtt_client_publish(mqtt_client, "/products/mattress/SWA002",post_data2, 0, 0, 0);
                                ESP_LOGI(TAG_MQTT, "[%d] Publishing...", msg_id);   
                                memset(post_data2,0,strlen(post_data2));  
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
		}
		else if(err == ESP_OK)
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
                ESP_LOGI(TAG1, "retry to connect to the AP");
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
            ESP_LOGI(TAG1,"connect to the AP fail");
            }
        else{//断线重连  密码错误尝试重新链接
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);//清除标志位
         ESP_LOGI(TAG1, "断线重连.........");
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
                           xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);}
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {//等待配网 第四步，等待手机配网
        ESP_LOGI(TAG1, "Scan done");
        xTaskCreate(&led_test_task, "led_test_task", 2048, NULL, 10, &led_test_task_Handler); 
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {//扫描信道 第五步，找到udp广播
        ESP_LOGI(TAG1, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) { //获取到ssid和密码   第六步，获取 wifi信息
        ESP_LOGI(TAG1, "Got SSID and password");
        vTaskSuspend(led_test_task_Handler);
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
            ESP_LOGI(TAG1, "wifi_init_sta finished.");
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
         ESP_LOGI(TAGTIME, "Time is not set yet. ");
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
    } else if (bits & WIFI_FAIL_BIT) {} 
    else {ESP_LOGE(TAG1, "UNEXPECTED EVENT");}
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
        {      ESP_LOGI(TAG1, "smartconfig over");
               esp_smartconfig_stop();
               ESP_LOGI(TAG1, "smartconfig 里面的任务");
               vTaskDelete(led_test_task_Handler);
                mqtt_app_start();
                xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, &adc1_get_data_task_Handler);
                xTaskCreate(&mqtt_task,"mqtt_task",8192,NULL,6,NULL);
                xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 7, NULL);
                vTaskDelete(NULL);     
        }
          vTaskDelay(1 / portTICK_PERIOD_MS);///freertos里面的延时函数
    }
}                     

void app_main(void)
{
    ESP_LOGI(TAG_MQTT, "[APP] Startup..");
    ESP_LOGI(TAG_MQTT, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_MQTT, "[APP] IDF version: %s", esp_get_idf_version());
    adc_init();//adc初始化
    gpio_pad_select_gpio(LED_G_IO);
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_G_IO, 0);
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
      adc_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!adc_semaphore) {
        ESP_LOGE(TAG1, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
    }    
     led_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!led_semaphore) {
        ESP_LOGE(TAG1, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
    }

     original_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!original_semaphore) {
        ESP_LOGE(TAG1, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
     }
          mqtt_semaphore = xSemaphoreCreateBinary();
    //要使用信号量达到两个任务先后执行，比如任务A执行初始化以后，给出信号量消息，然后任务B才运行
    if (!mqtt_semaphore) {
        ESP_LOGE(TAG1, "%s, init fail, the adc semaphore create fail.", __func__);
        return;
     }
    ESP_ERROR_CHECK(nvs_flash_init());
     initialise_wifi();

    mqtt_app_start();
    xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, &adc1_get_data_task_Handler);
    xTaskCreate(&mqtt_task,"mqtt_task",8192,NULL,6,NULL);
    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 7, NULL);
}

void Get_Current_Original_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 成人
    cJSON_AddNumberToObject(root,"breathFlag",HTTP_Body_Data[0]);//状态标志位 1正常 2 呼吸暂停 3低通气 4 呼吸过快 5 不通气6 在床 7 离床 8 体动
    cJSON_AddNumberToObject(root,"breathRate",HTTP_Body_Data[1]);//呼吸频率
    cJSON_AddNumberToObject(root,"breathAvg",HTTP_Body_Data[2]);//呼吸平均值
    cJSON_AddNumberToObject(root,"heartRate",HTTP_Body_Data[3]);//心率
    cJSON_AddNumberToObject(root,"heartAvg",HTTP_Body_Data[4]);//心率平均值
    cJSON_AddNumberToObject(root,"bodyTP",HTTP_Body_Data[5]);//体温
    cJSON_AddStringToObject(root,"pushTime",cname);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

void Get_Current_Body_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    cJSON *array=NULL;
    root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"product_model","MS001");// 成人
    cJSON_AddStringToObject(root,"user-id","HF102002-A");// 成人
    cJSON_AddNumberToObject(root,"breath",MQTT_Body_Data[0]);//呼吸
    cJSON_AddNumberToObject(root,"heart",MQTT_Body_Data[1]);//心率
    cJSON_AddNumberToObject(root,"sleepFlag",MQTT_Body_Data[2]);//睡眠标志位
    cJSON_AddItemToObject(root,"waring",array=cJSON_CreateArray());  
    cJSON *ArrayItem0=cJSON_CreateObject();
     cJSON_AddStringToObject(ArrayItem0,"t",waring_string);
     cJSON_AddNumberToObject(ArrayItem0,"w",waring_flag);
    cJSON_AddItemToArray(array,ArrayItem0);

    cJSON_AddStringToObject(root,"time",cname);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

void Get_Current_monitorSleep_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    cJSON *array=NULL;
    cJSON *array1=NULL;
    cJSON *array2=NULL;
    root =cJSON_CreateObject();
    array =cJSON_CreateObject();
    array1 =cJSON_CreateObject();
    array2 =cJSON_CreateObject();
    cJSON_AddItemToObject(root,"user-id",cJSON_CreateString("HF102001-A")); 
    cJSON_AddItemToObject(root,"abnormal",array1);
    cJSON_AddStringToObject(array1,"type",waring_string);
    cJSON_AddStringToObject(array1,"time",cname);
    //cJSON_AddItemToObject(root,"service",array2);
    //cJSON_AddStringToObject(array2,"type",waring_type);
    //cJSON_AddStringToObject(array2,"time",cname);
    cJSON_AddStringToObject(root,"time",month);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

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
    if(retry==10)
    esp_restart();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);//获取网络时间，64bit的秒计数
    localtime_r(&now, &timeinfo);//转化为具体的时间参数
}
static void adc1_get_data_task(void *pvParameters)
{    
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
    int people_counter = 9; //重物判断参数
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
       	//1分钟内的所有呼吸率和心率值
	double * resp_1_minute = (double *)malloc(24*sizeof(double));
	double * heart_1_minute = (double *)malloc(24*sizeof(double));
	double * heart_10_minute = (double *)malloc(240*sizeof(double));
    uint64_t count_time = 0;
	int count_t = 0;
	int count_stage = 0;
    double event_type_vec;
    int log_level = 0;
       //===========一些大循环里用的变量=============== [
		//实际上应该用一个循环队列，因为只有数组大小为2，所以简化了
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
        vTaskDelay(18/ portTICK_PERIOD_MS);
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
                     time_t now;
                     struct tm timeinfo;
                     time(&now);
                     localtime_r(&now, &timeinfo);
                     if (timeinfo.tm_year < (2016 - 1900)) {
                     obtain_time();
                     time(&now);
                     }
                    char strftime_buf[64];
                    char strftime_month_buf[64];
                    setenv("TZ", "CST-8", 1);
                    tzset();
                    struct tm *tm=localtime_r(&now, &timeinfo);//获取系统时间
                    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d  %H:%M:%S", &timeinfo);//组建时间戳
                    strftime(strftime_month_buf, sizeof(strftime_month_buf), "%Y-%m", &timeinfo);//组建时间戳
                    cname=strftime_buf;
                    month=strftime_month_buf;
                    current_hour = tm->tm_hour;//获取当前小时
                    current_min =tm->tm_min;//获取当前分钟
                    if(data_index == 0) {
                            if(read_raw2 < 1000) {
                                output_data = 0;
                            }
                            if(output_data < 10)	{
                                    begin3 = 0;
                                    continue;
                            }
                        }
                       data_index++;
                      // printf("pv: %d   pr: %d   \n ",read_raw1,read_raw2);
                     }
            X_X_heart[begin3++] = read_raw11 - read_raw33;//每次都先取一些数据放在这个数组里面
             if(time1 % sample_points != 0)
             continue;//取原始数据，取100次，跳出本次循环100次
             time1 = 0;//取完成以后，标志位清零
             current_period = IN_BED;
        if(data_index <= N_seg){
            X_X[begin1] = read_raw1 - read_raw3;
            Pr_X[begin1] = read_raw2;
            begin1++;
            //printf("\n%d\n%d\n",begin1,begin3);
            if(begin1==N_seg){
                begin1 = 0;
                begin3 = 0;
            }
        }
            //将上一个75点的呼吸率数据备份
            Data_transfer(X_X_1, X_X ,N_seg - N_seg_1, N_seg);
            Data_transfer(Pr_X_1, Pr_X ,N_seg - N_seg_1, N_seg);
            //将上一个750点的呼吸率心率数据备份
            Data_transfer(X_X_heart_1, X_X_heart , N_heart - N_heart_1, N_heart);
            //printf("Sleep after two minutes:\n");
            if(data_index > N_seg){
            current_period = AFTER_ADAPT; 
            X_X_2[begin2] = read_raw1 - read_raw3; //压电取一百个原始数据，当大于100个数据时，开始进行计算
            Pr_X_2[begin2] = read_raw2; //压阻
            begin2++;
            if(begin2==N_seg - N_seg_1){//如果取了 2.5的数据
            count_time++;
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
                wavedec(X_X, out_sign, out_levels, N_seg, 5);
                wrcoef(out_sign,out_levels,a2_resp);
                free(out_sign);out_sign = NULL;
                free(out_levels);out_levels = NULL;
                //计算压阻绝对值之和
                seg_pre_abssum = onearray_sum_fabs(Pr_X,N_seg);
                //计算能量值
                seg_energy = accumulate1(a2_resp,N_seg,N_seg);
            if(seg_pre_abssum < 1000)
             {     
                	printf("has left bed\n");
                    if((current_hour>=start_time)||(current_hour<=stop_time))//晚八点到早八点时间限制
                   {   
                       if(out_bed_flag==4)//已经发送过警告到服务器端
                       out_bed_flag=4;//持续等待不再发送数据,直到出现体动、在床时该标志位才会刷新
                       if((out_bed_flag==3)||(out_bed_flag==2))
                       out_bed_flag=2;        //表示已经记录过离床时间戳,2表示持续离床状态
                       if(out_bed_flag==0) //检测到离床
                       out_bed_flag=1;
                       printf("current time is : %d \n",current_hour);
                       printf("within time limits ! \n");
                   }
                    else
                  {  out_bed_flag=0;//不在报警时间内
                      printf("current time is : %d  \n",current_hour);
                      printf("out of time ! \n");
                  }
                   if(out_bed_flag==1)//在报警时间范围内　　确认用户离床
                 {       
                    min_of_out_bed=current_min;//记录离床时间点，记录完成后，本次离床过程中，时间戳不再更新
                    out_bed_flag=3;//离床标志为记录为3代表，已经记录离床时间戳。
               }
                   if(out_bed_flag==2)
                {       
                    if(min_of_out_bed<=29)
                    {
                    lengthtime=current_min-min_of_out_bed;//记录离床时间点
                    }
                    else
                    {    if(current_min<=59)//如果分钟数没超过59
                         lengthtime=current_min-min_of_out_bed;//记录离床时间点
                         else//分钟数超过59，从零开始计算，所以基数要加 60
                         {
                             lengthtime=(current_min+60)-min_of_out_bed;            
                         }    
                    }
                }
              if((lengthtime>=LEFT_TIME_OF_OUT_BED)&&(out_bed_flag==2))
             {         waring_string="timeout_off_bed";
                        waring_type="Need to see";
                        waring_flag=1;      
                        out_bed_flag=4;//标志为4,说明警告已经发送完成
                        http_data_type_flag=1;
                        xSemaphoreGive(adc_semaphore);
                        vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                        xSemaphoreGive(mqtt_semaphore);
                        vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给mqtt任务
             }    
                    waring_string="bed_off";
                    waring_flag=0;    
                	//做一些reset工作
                    datanum_after_inbed = 0;
                	//清理之前分配的内存
                	free(a2_resp);a2_resp = NULL;
					//free(a2_heart);a2_heart = NULL;
					finish = clock();
                    duration = (double)(finish - start) / CLOCKS_PER_SEC;
                    MQTT_Body_Data[0]=0;
                    MQTT_Body_Data[1]=0;
                    MQTT_Body_Data[2]=3;
                    HTTP_Body_Data[1]=0;
                    HTTP_Body_Data[3]=0;
                    HTTP_Body_Data[0]=7;
                   http_data_type_flag=2;
                   xSemaphoreGive(adc_semaphore);
                   vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                   xSemaphoreGive(mqtt_semaphore);
                   vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给mqtt任务
                	continue;
                }
             if(judge_whether_human(X_X,N_seg) == 1){
                    resp_value = 0.;
                    heart_value = 0.;
                    not_people_counter = not_people_counter + 1;
					if(not_people_counter>=4) people_counter = 0;
                }
                else{
                    not_people_counter = 0;
                    people_counter =  people_counter +1;
                }
                if(not_people_counter >= 4 || people_counter <=12) {
        
                     printf("it is not human\n");
                    datanum_after_inbed = 0;
                    //清理之前分配的内存
                    free(a2_resp);a2_resp = NULL;
                    //free(a2_heart);a2_heart = NULL;
                    finish = clock();
                    duration = (double)(finish - start) / CLOCKS_PER_SEC;
                       if((current_hour>=20)||(current_hour<=7))//晚八点到早八点时间限制
                   {   
                       if(out_bed_flag==4)//已经发送过警告到服务器端
                       out_bed_flag=4;//持续等待不再发送数据
                       if((out_bed_flag==3)||(out_bed_flag==2))
                       out_bed_flag=2;        //表示已经记录过离床时间戳,2表示持续离床状态
                       if(out_bed_flag==0) //检测到离床
                       out_bed_flag=1;
                       printf("current time is : %d \n",current_hour);
                       printf("within time limits ! \n");
                   }
                    else
                  {  out_bed_flag=0;//不在报警时间内
                      printf("current time is : %d  \n",current_hour);
                      printf("out of time ! \n");
                  }
                   if(out_bed_flag==1)//确定　为离床
                 {       
                    min_of_out_bed=current_min;//记录离床时间点
                    out_bed_flag=3;//离床标志为记录为3代表，已经记录离床时间戳。
               }
                   if(out_bed_flag==2)
                {       
                    if(min_of_out_bed<=29)
                    {
                    lengthtime=current_min-min_of_out_bed;//记录离床时间点
                    }
                    else
                    {    if(current_min<=59)//如果分钟数没超过59
                         lengthtime=current_min-min_of_out_bed;//记录离床时间点
                         else//分钟数超过59
                         {
                               lengthtime=(current_min+60)-min_of_out_bed;            
                         }    
                    }
                }
                    if((lengthtime>=LEFT_TIME_OF_OUT_BED)&&(out_bed_flag==2))
                    {        
                                waring_string="timeout_off_bed";
                                 waring_type="Need to see";
                                 waring_flag=1;
                                out_bed_flag=4;//标志为4,说明警告已经发送完成
                                http_data_type_flag=1;
                                xSemaphoreGive(adc_semaphore);
                                vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                   
                                 xSemaphoreGive(mqtt_semaphore);
                                 vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给mqtt任务
                    }    
                     waring_string="bed_off";
                    waring_flag=0;   
                	//做一些reset工作
                    datanum_after_inbed = 0;
                	//清理之前分配的内存
                	free(a2_resp);a2_resp = NULL;
					//free(a2_heart);a2_heart = NULL;
					finish = clock();
                    duration = (double)(finish - start) / CLOCKS_PER_SEC;
                    MQTT_Body_Data[0]=0;
                    MQTT_Body_Data[1]=0;
                    MQTT_Body_Data[2]=3;
                    HTTP_Body_Data[1]=0;
                    HTTP_Body_Data[3]=0;
                    HTTP_Body_Data[0]=7;
                   http_data_type_flag=2;
                   xSemaphoreGive(adc_semaphore);
                   vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                   xSemaphoreGive(mqtt_semaphore);
                   vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给mqtt任务
                    continue;
                }
                	//计算压阻一阶导数绝对值的和
				  double * out_diff = (double *)malloc(N_seg*sizeof(double));
				  diff_fabs(Pr_X,N_seg,out_diff,&out_num);
				  pr_diff = onearray_sum(out_diff,out_num);
				  free(out_diff);out_diff = NULL;
				  if(pr_diff >= 1000){
					printf("there is body movement\n");
                    out_bed_flag=0;
               
                    //做一些reset工作
                    datanum_after_inbed = 0;
					//清理之前分配的内存
					free(a2_resp);a2_resp = NULL;
					//free(a2_heart);a2_heart = NULL;
					finish = clock();
					duration = (double)(finish - start) / CLOCKS_PER_SEC;
					//printf( "\n算法时间：%f seconds\n", duration );
                    MQTT_Body_Data[0]=0;
                    MQTT_Body_Data[1]=0;
                    MQTT_Body_Data[2]=0;
                    HTTP_Body_Data[1]=0;
                    HTTP_Body_Data[3]=0;
                    HTTP_Body_Data[0]=8;
                    http_data_type_flag=2;
                    waring_string="body_moving";
                    waring_flag=0;
                    xSemaphoreGive(adc_semaphore);
                     vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                    xSemaphoreGive(mqtt_semaphore);
                    vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
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
                else if (enable_further_judege == 1)	//根据过往信息进一步判断
                {
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
                  printf("resp_value = %f,heart_value = %f\n",resp_value,heart_value);
                  out_bed_flag=0;
                  MQTT_Body_Data[0]=(int)resp_value;
                  MQTT_Body_Data[1]=(int)heart_value;
                  MQTT_Body_Data[2]=1;
                  HTTP_Body_Data[1]=(int)resp_value;
                  HTTP_Body_Data[3]=(int)heart_value;
                  HTTP_Body_Data[0]=6;
                  http_data_type_flag=2;
                  waring_string="bed_touch";
                  waring_flag=0;
                  xSemaphoreGive(adc_semaphore);
                  vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
                  xSemaphoreGive(mqtt_semaphore);
                  vTaskDelay(500 / portTICK_PERIOD_MS);//预留时间给http任务
			
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
    free(heart_10_minute);heart_10_minute=NULL;
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

static void wifi_led_test_task(void *pvParameters)//wifi断开
{
        for(int i=0;i<5;i++){
            gpio_set_level(LED_G_IO, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数  
            gpio_set_level(LED_G_IO, 0);  
            vTaskDelay(100 / portTICK_PERIOD_MS);///freertos里面的延时函数
            }
             vTaskDelete(NULL);
}

void make_further_polishment(double absmax,
                             double *p_resp_value, double *p_heart_value,
                                int *p_resp_exception_counter, int *p_heart_exception_counter,
                                history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos) {
	double history_resp_sum = 0.;
	double history_heart_sum = 0.;
	uint8_t field = -1;
		 //======================呼吸率================================ [
		 //std函数比较费时间，暂时不用
		 //uint8_t condition_outliers = (absmax_vec(i) > 2*adaptive_absmax_normal || absmax_vec(i) - mean(X_seg) > 3*std(X_seg));
	 //uint8_t condition_outliers = (absmax > 2*adaptive_absmax_normal );
     uint8_t whether_resp_abnormal = (*p_resp_value > 35 || *p_resp_value < 10);
     if(whether_resp_abnormal == 1) {//异常的呼吸率
//            if(condition_outliers == 1) { //且存在异常点,放弃本次呼吸率，用过往历史
//            		field = 3;
//            		history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
//                *p_resp_value = history_resp_sum/(front_pos - 0 + 1);
//            }
//            else { //没有异常点
                //To do，目前也是放弃本次测量。用一个计数器累积，如果连续3次这种情况，就报告出去
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
            //}
      }
     if(whether_resp_abnormal == 0) { //正常的呼吸率,用当前和过往的做平均
     				field = 3;
            history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
            *p_resp_value = (history_resp_sum + *p_resp_value)/((front_pos - 0 + 1) + 1);
            *p_resp_exception_counter = 0;
     }
     // ========================================================= ]

     //======================心率================================ [
     uint8_t whether_heart_abnormal = (*p_heart_value > 150 || *p_heart_value < 50);
     if(whether_heart_abnormal == 1 ) //异常的心率
     {
//     		if(condition_outliers == 1) //且存在异常点, 放弃本次心率，用过往历史
//     		{
//     			field = 4;
//          history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
//          *p_heart_value = history_heart_sum/(front_pos - 0 + 1);
//     		}
//     		else  //没有异常点
//     		{
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
