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
#include "driver/dac.h"
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
#define webaddress  "http://112.95.150.108:20005/originalMonitor.action"
const char *webaddress1[3]={"http://112.95.150.108:20005/originalMonitor.action","http://112.95.150.108:20005/monitor.action",0};
/**********************************低功耗相关头文件**********************/
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"

#define   SAVE_POWER_WAIT_TIME     180//180S
static RTC_DATA_ATTR struct timeval sleep_enter_time;
/**********************************低功耗相关文件**********************/
/**********************************低功耗相关文件**********************/
/**********************************低功耗相关文件**********************/
/**********************************低功耗相关文件**********************/
/**********************************低功耗相关文件**********************/
static void low_vol_protect_task(void *arg)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
   //ESP_ERROR_CHECK( esp_wifi_stop());
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

    //const int wakeup_time_sec = 20;
  // printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);//使能定时器timer
   //esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    //const int ext_wakeup_pin_1 = 34;
 //   const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
    const int ext_wakeup_pin_2 = 35;
    const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;
   // printf("Enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n", ext_wakeup_pin_1, ext_wakeup_pin_2);
 //   esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
       printf("Enabling EXT1 wakeup on pins GPIO%d\n", ext_wakeup_pin_2);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    printf("Entering deep sleep\n");
    gettimeofday(&sleep_enter_time, NULL);
    esp_deep_sleep_start();
     vTaskDelete(NULL);
}
   


short http_send_flag=0,low_power_flag=0;
char wifi_flag=0;
char http_send_fail=0;
short person_flag=0;
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
/**********************************ota相关文件**********************/
/**********************************ota相关文件**********************/
/**********************************ota相关文件**********************/
/**********************************ota相关文件**********************/
/**********************************ota相关文件**********************/
//配置信息

#define EXAMPLE_SERVER_IP   "192.168.3.10"
#define EXAMPLE_SERVER_PORT "8000"
#define EXAMPLE_FILENAME "/adc2.bin"
//数据包长度
#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024
static const char *TAGOTA = "ota";
//OTA数据
static char ota_write_data[BUFFSIZE + 1] = { 0 };
//接收数据
static char text[BUFFSIZE + 1] = { 0 };
//镜像大小
static int binary_file_length = 0;
//socket句柄
static int socket_id = -1;
/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char *buffer, char delim, int len)
{
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
static bool read_past_http_header(char text[], int total_len, esp_ota_handle_t update_handle)
{
    /* i means current position */
    int i = 0, i_read_len = 0;
    while (text[i] != 0 && i < total_len) {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write( update_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAGOTA, "Error: esp_ota_write failed! err=0x%x", err);
                return false;
            } else {
                ESP_LOGI(TAGOTA, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }
            return true;
        }
        i += i_read_len;
    }
    return false;
}

static bool connect_to_http_server()
{
    ESP_LOGI(TAGOTA, "Server IP: %s Server Port:%s", EXAMPLE_SERVER_IP, EXAMPLE_SERVER_PORT);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;
    //新建socket
    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1) {
        ESP_LOGE(TAGOTA, "Create socket failed!");
        return false;
    }

    //设置连接参数
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(EXAMPLE_SERVER_IP);
    sock_info.sin_port = htons(atoi(EXAMPLE_SERVER_PORT));

    //连http服务器
    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1) {
        ESP_LOGE(TAGOTA, "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return false;
    } else {
        ESP_LOGI(TAGOTA, "Connected to server");
        return true;
    }
    return false;
}
//异常处理，连接http服务器失败等异常
static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAGOTA, "Exiting task due to fatal error...");
    close(socket_id);
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

//OTA任务
static void ota_example_task(void *pvParameter)
{
    esp_err_t err;
    ESP_LOGI(TAGOTA, "1");
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAGOTA, "Starting OTA example...");
    //获取当前boot位置
    const esp_partition_t *configured = esp_ota_get_boot_partition();
     ESP_LOGI(TAGOTA, "2");
    //获取当前系统执行的固件所在的Flash分区
    const esp_partition_t *running = esp_ota_get_running_partition();
     ESP_LOGI(TAGOTA, "2");

    if (configured != running) {
        ESP_LOGW(TAGOTA, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAGOTA, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAGOTA, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    ESP_LOGI(TAGOTA, "Connect to Wifi ! Start to Connect to Server....");

    //连http服务器
    if (connect_to_http_server()) {
        ESP_LOGI(TAGOTA, "Connected to http server");
         ESP_LOGI(TAGOTA, "5");
    } else {
        ESP_LOGE(TAGOTA, "Connect to http server failed!");
        task_fatal_error();
         ESP_LOGI(TAGOTA, "6");
    }

    //组http包发送
    const char *GET_FORMAT =
        "GET %s HTTP/1.0\r\n"
        "Host: %s:%s\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n\r\n";

    char *http_request = NULL;
    int get_len = asprintf(&http_request, GET_FORMAT, EXAMPLE_FILENAME, EXAMPLE_SERVER_IP, EXAMPLE_SERVER_PORT);
    if (get_len < 0) {
        ESP_LOGE(TAGOTA, "Failed to allocate memory for GET request buffer");
        task_fatal_error();
    }
    int res = send(socket_id, http_request, get_len, 0);
    free(http_request);
    if (res < 0) {
        ESP_LOGE(TAGOTA, "Send GET request to server failed");
        task_fatal_error();
    } else {
        ESP_LOGI(TAGOTA, "Send GET request to server succeeded");
    }

    //获取当前系统下一个（紧邻当前使用的OTA_X分区）可用于烧录升级固件的Flash分区
    update_partition = esp_ota_get_next_update_partition(NULL);
     ESP_LOGI(TAGOTA, "6");
    ESP_LOGI(TAGOTA, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);
    //OTA写开始
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
     ESP_LOGI(TAGOTA, "7");
    if (err != ESP_OK) {
        ESP_LOGE(TAGOTA, "esp_ota_begin failed, error=%d", err);
        task_fatal_error();
    }
    ESP_LOGI(TAGOTA, "esp_ota_begin succeeded");

    bool resp_body_start = false, flag = true;
    //接收完成
    while (flag) {
        memset(text, 0, TEXT_BUFFSIZE);
        memset(ota_write_data, 0, BUFFSIZE);
        //接收http包
        int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);
        if (buff_len < 0) { //包异常
            ESP_LOGE(TAGOTA, "Error: receive data error! errno=%d", errno);
            task_fatal_error();
        } else if (buff_len > 0 && !resp_body_start) { //包头
            memcpy(ota_write_data, text, buff_len);
            resp_body_start = read_past_http_header(text, buff_len, update_handle);
        } else if (buff_len > 0 && resp_body_start) { //数据段包
            memcpy(ota_write_data, text, buff_len);
            //写flash
            err = esp_ota_write( update_handle, (const void *)ota_write_data, buff_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAGOTA, "Error: esp_ota_write failed! err=0x%x", err);
                task_fatal_error();
            }
            binary_file_length += buff_len;
            ESP_LOGI(TAGOTA, "Have written image length %d", binary_file_length);
        } else if (buff_len == 0) {  //结束包
            flag = false;
            ESP_LOGI(TAGOTA, "Connection closed, all packets received");
            close(socket_id);
        } else {//未知错误
            ESP_LOGE(TAGOTA, "Unexpected recv result");
        }
    }

    ESP_LOGI(TAGOTA, "Total Write binary data length : %d", binary_file_length);
    //OTA写结束
    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAGOTA, "esp_ota_end failed!");
        task_fatal_error();
    }
    //升级完成更新OTA data区数据，重启时根据OTA data区数据到Flash分区加载执行目标（新）固件
    err = esp_ota_set_boot_partition(update_partition);
     ESP_LOGI(TAGOTA, "8");
    if (err != ESP_OK) {
        ESP_LOGE(TAGOTA, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }
    ESP_LOGI(TAGOTA, "Prepare to restart system!");
    esp_restart();
     ESP_LOGI(TAGOTA, "9");
    return ;
}

/**********************************ota结束相关文件**********************/
/**********************************ota结束相关文件**********************/
/**********************************ota结束相关文件**********************/
/**********************************ota结束相关文件**********************/
/**********************************ota结束相关文件**********************/
/**************************sntp相关定义***********当前时间***********************************************/
/* ISO C `broken-down time' structure.  */
static const char *TAGTIME = "TIME";
static void obtain_time(void);
char *cname;
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
//smartconfig  2020.4.14
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
int  HTTP_Send_Data[20]={0};
/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
//http 发送数据  2020.4.14


extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");
void Get_CurrentData(char *dat)
{
     cJSON *root=NULL;//定义一个指针变量
     root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId","F8D335EA8F2D4D52059A91F73EA7463C");//3c
    cJSON_AddStringToObject(root,"pushTime",cname);
    cJSON *array=NULL;
    //创建一个数组
    cJSON_AddItemToObject(root,"original",array=cJSON_CreateArray());  
    //在数组上添加对象
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
        .url = webaddress1[0],
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);//用户客户端初始化
    char post_data1[1024]={0};
    Get_CurrentData(post_data1);//得到json数据，用如post data
    // POST
    esp_http_client_set_url(client, webaddress1[0]);//接口地址
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
            esp_restart();
        }
    }
    esp_http_client_cleanup(client);
}
static void http_test_task(void *pvParameters)
{
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
static void adc1_get_data_task(void *pvParameters)
{    
     /*   uint32_t retry =0;
    time_t now=0;
    time_t return_now;
    struct tm timeinfo;

    char strftime_buf[64];
    timeinfo.tm_year = 0;*/
 
    
    uint64_t tt = 0;
    int output_data=0;
    uint8_t time1=0;
    uint32_t num1=0,num2=0,num3=0;
    int read_raw1, read_raw2, read_raw3;
    uint64_t data_index = 0;
    int begin1 = 0;
    int begin2 = 0;
    int N_seg = 100;//每段信号100个数据点
    //int N_overlay = 30;//重采样
    int adative_range = 12;//最开始的2分钟 12*10 = 2 分钟
    double adaptive_energy_thresh = -1;//用来检测是否有体动
    double adaptive_resp_normal = -1; //用来检测正常的呼吸频率
    //int Total_seg = (row-N_overlay)/(N_seg-N_overlay);//总的段数
    double seg_energy_vec;
    double rate_value_vec = 0.;
    double rate_value_vec1;
    double rate_value_vec2;
    int adative_range_index = 0;
    /************************************************************/
    /************************************************************/
    /************************************************************/
   /************************************************************/
   /************************************************************/
   /************************************************************/
   /*********************获取标准时间**************************/



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

	//esp_err_t err = esp_timer_create(&fw_timer, &fw_timer_handle);//创建定时器任务
    //printf("start conversion.\n");
    printf("Sleep after two minutes:");
    while(1) {

        //vTaskDelay(32/ portTICK_PERIOD_MS);
        //event_type_vec = 0;
       //usleep(625);
       ets_delay_us(1250);
       // vTaskDelay(1/ portTICK_PERIOD_MS);
       //定时器创建、启动

	  //err = esp_timer_start_periodic(fw_timer_handle, 1250*1000);//625us秒回调
        time1++;
        tt++;
        read_raw1 = adc1_get_raw( ADC1_CHANNEL_6);//压电
        read_raw2 = adc1_get_raw( ADC1_CHANNEL_7);//压阻
        read_raw3 = adc1_get_raw( ADC1_CHANNEL_5);//VREF
        num1=num1+read_raw1;
        num2=num2+read_raw2;
        num3=num3+read_raw3;
        if (time1==80) {
            output_data++;
            time1=0;
            read_raw1=num1/80;
            read_raw2=num2/80;
            read_raw3=num3/80;
            num1=0;num2=0;num3=0;
            read_raw1=read_raw1-read_raw3;
            
           HTTP_Send_Data[http_send_flag*2]=read_raw1;
           HTTP_Send_Data[http_send_flag*2+1]=read_raw2;

           http_send_flag++;
            if(http_send_flag==10)
            { 
            if(HTTP_Send_Data[19]>2000)person_flag++;
                low_power_flag++;
             if(low_power_flag>=SAVE_POWER_WAIT_TIME)
            {
                low_power_flag=0; 
                if(person_flag<(SAVE_POWER_WAIT_TIME/2))
                  {person_flag=0;
                     //进入低功耗
                     xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 8, NULL);
                     // xTaskCreate(low_vol_protect_task, "low_vol_protect_task", 2048, NULL, 6, NULL);
                  }
                 else  {person_flag=0;}

              }
                
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
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);
    cname=strftime_buf;


/**************************sntp时间戳初始化结束************************************/



                 xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
                 http_send_flag=0;
            }
            printf("%d:      PV:  %d      PR:  %d   vref : %d   \n", output_data, read_raw1,read_raw2,read_raw3);
            data_index++;
        }
    }
  vTaskDelete(NULL);
}
//wifi函数
static EventGroupHandle_t  wifi_event_group;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG2 = "wifi station";
static int s_retry_num = 0;
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? *///我们只关心一件事，是否通过ip地址链接上一个热点
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
    
     
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  
            ESP_LOGI(TAG, "wifi_init_sta finished.");
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
      //  ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
      //           EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
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
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAGTIME, "The current date/time in hefei is: %s", strftime_buf);




/**************************wsntp时间戳初始化结束************************************/





    } else if (bits & WIFI_FAIL_BIT) {
     //   ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
      //           EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
     xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, NULL);
    vEventGroupDelete(s_wifi_event_group);

}

/*smartconfig任务
* @param[in]   void  		       :wu
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
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
             xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, NULL);
            vTaskDelete(NULL);     
        }
    
    }
}


  


/******************smartconfig   over************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

//定时器句柄
esp_timer_handle_t fw_timer_handle = 0;

void fw_timer_cb(void *arg) 
{
	//获取时间戳
	int64_t tick = esp_timer_get_time();
	//printf("timer cnt = %lld \r\n", tick);
		//定时器暂停、删除
		esp_timer_stop(fw_timer_handle);
		//esp_timer_delete(fw_timer_handle);
		//printf("timer stop and delete!!! \r\n");
		//重启
		//esp_restart();
}

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
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
      initialise_wifi();//wifi链接
    
       
    
        

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

    //ESP_ERROR_CHECK( example_disconnect() );
}
