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
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"

/**********************************低功耗相关头文件**********************/
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
/***************************算法相关**********************/
void make_further_polishment(double pr_diff, double absmax, double adaptive_absmax_normal,
														 double *p_resp_value, double *p_heart_value,
															int *p_resp_exception_counter, int *p_heart_exception_counter,
															history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos);
 

/***************************算法相关**********************/
/**********************************低功耗相关头文件**********************/
#define   SAVE_POWER_WAIT_TIME    300//5分钟
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
/**********************************ota相关文件**********************/
//配置信息
#define EXAMPLE_SERVER_IP   "192.168.3.10"
#define EXAMPLE_SERVER_PORT "8000"
#define EXAMPLE_FILENAME "/adc1.bin"
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
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    ESP_LOGI(TAGOTA, "Starting OTA example...");
    //获取当前boot位置
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    //获取当前系统执行的固件所在的Flash分区
    const esp_partition_t *running = esp_ota_get_running_partition();
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
    ESP_LOGI(TAGOTA, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);
    //OTA写开始
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
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
    if (err != ESP_OK) {
        ESP_LOGE(TAGOTA, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }
    ESP_LOGI(TAGOTA, "Prepare to restart system!");
    esp_restart();
    return ;
}

/**********************************ota结束相关文件**********************/
/**************************sntp相关定义***********当前时间***********************************************/
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
    int output_data=0;
    uint32_t num1=0,num2=0,num3=0;
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
    int N_overlay = 30;
    //最开始的2分钟 12*10 = 2 分钟
    int adative_range = 12;
    //正常的信号能量
    double adaptive_energy_thresh = -1;
    //正常的呼吸频率
    double adaptive_resp_normal = -1;
    //正常的心率
    double adaptive_heart_normal = -1;
    //正常的压阻变化绝对值
    double adaptive_prdiff_normal = -1;
    //正常的信号绝对值最大值
    double adaptive_absmax_normal = -1;

    double seg_energy = 0.;
    double seg_pre_abssum = 0.;
    double resp_value = 0.;
    double heart_value = 0.;
    double pr_diff = 0.;
    double absmax = 0.;

    int adative_range_index = 0;

    double * median_energy_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_energy_vec, 0x00, adative_range*sizeof(double));

    double * median_resp_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_resp_vec, 0x00, adative_range*sizeof(double));

    double * median_heart_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_heart_vec, 0x00, adative_range*sizeof(double));

    double * median_prdiff_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_prdiff_vec, 0x00, adative_range*sizeof(double));

    double * median_absmax_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_absmax_vec, 0x00, adative_range*sizeof(double));

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
	 //================================================ ]
    while(1) {  
        event_type_vec = 0;
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
            if(current_period == IDLE && log_level < 1)
            	//printf("%d      %d      %d      %d      %d  \n", current_period, output_data, read_raw1,read_raw2, read_raw3 );
            read_raw1=num1/sample_points;
            read_raw2=num2/sample_points;
            read_raw3=num3/sample_points;
            num1=0;num2=0;num3=0;
           read_raw111=read_raw1-read_raw3;
           HTTP_Send_Data[http_send_flag*2]=read_raw111;
           HTTP_Send_Data[http_send_flag*2+1]=read_raw2;
            http_send_flag++;
            if(http_send_flag==10)
            {        time_t now;
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
                    http_data_type_flag=1;
                    xSemaphoreGive(adc_semaphore);//满足条件时，上传原始数据,释放二值信号量
                     http_send_flag=0;//标志位清零
                       if(read_raw2<2000) low_power_flag++; else low_power_flag=0; //低功耗相关
                   if(low_power_flag>=SAVE_POWER_WAIT_TIME)//如果满足条件，就进入低功耗
                {low_power_flag=0;
                xTaskCreate(&low_vol_protect_task, "low_vol_protect_task", 2048, NULL, 6,&low_power_Handler);  
                }
            }
            //by xiayi, 连续10个采样点有足量压阻才算数===[
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
        }

        X_X_heart[begin3++] = read_raw11 - read_raw33;//取压阻原始数据
        if(time1 % sample_points != 0)
            continue;
        time1 = 0;
//开始的信号不用，用于获取一些自适应的参数，取前两分钟
        //printf("person in bed detected \n");
        current_period = IN_BED;
 if(data_index <= adative_range*N_seg){
            //printf("%llu:      PV:  %d      PR:  %d      VREF:  %d  \n", data_index, read_raw1,read_raw2, read_raw3 );
            if(current_period == IN_BED && log_level < 1)
            //	printf("%d      %llu      %d      %d      %d  \n", current_period, data_index, read_raw1,read_raw2, read_raw3 );
            X_X[begin1] = read_raw1 - read_raw3;
            Pr_X[begin1] = read_raw2;
            begin1++;
            //printf("\n%d\n%d\n",begin1,begin3);
                if(begin1==N_seg){
                begin1 = 0;
                begin3 = 0;
                double * out_sign = (double *)malloc(143*sizeof(double));
                int * out_levels = (int *)malloc(5*sizeof(int));
                double * a1_resp = (double *)malloc(N_seg*sizeof(double));
                //double * a1_heart = (double *)malloc(N_heart*sizeof(double));
                wavedec(X_X, out_sign, out_levels, N_seg, 5);//小波重构函数MATLAB中小波重构函数waverec。
//它能够实现对信号的重构，并且能够解决上述系数长度不匹配的问题（虽然我还没搞懂它是怎么解决这一问题的，呵呵）。
//想要使用这个函数，就必须先弄清waverec需要的矩阵C和L中存储的是什么东西。然后将得到的一系列阈值化后的CA和CD组合成C，并由分解过程得到矩阵L
                wrcoef(out_sign,out_levels,a1_resp);//进行小波变换
                free(out_sign);out_sign = NULL;//释放空间
                free(out_levels);out_levels = NULL;//释放空间

                //计算信号压电能量
                median_energy_vec[adative_range_index] = accumulate1(a1_resp,N_seg,N_seg);//MEDIAN 函数是一种计算机函数，能够返回给定数值的中值，
                //中值是在一组数值中居于中间的数值，如果参数集合中包含偶数个数字，函数 MEDIAN 将返回位于中间的两个数的平均值
                //计算呼吸频率
                median_resp_vec[adative_range_index] = obtain_resprate_with_max_possibility(a1_resp,N_seg);
                //计算心率
                median_heart_vec[adative_range_index] = obtain_heart_by_spec(X_X_heart,N_heart);
                printf("resp_value = %f,heart_value = %f\n",median_resp_vec[adative_range_index],median_heart_vec[adative_range_index]);

                // 计算压阻一阶导数绝对值的和
                double * out_diff = (double *)malloc(N_seg*sizeof(double));
                diff_fabs(Pr_X,N_seg,out_diff,&out_num);//fabs函数是一bai个求绝对值du的函数，求出x的绝对值
                median_prdiff_vec[adative_range_index] = onearray_sum(out_diff,out_num);
                free(out_diff);out_diff = NULL;

                //计算压电信号最大值
                max_v_interval_fabs(X_X,max_value_index,N_seg);
                median_absmax_vec[adative_range_index] = max_value_index[0];

                adative_range_index++;
                free(a1_resp);a1_resp = NULL;
                //free(a1_heart);a1_heart = NULL;
            }
            if(data_index==adative_range*N_seg){
                adaptive_energy_thresh = median(median_energy_vec,adative_range);
                adaptive_resp_normal = median(median_resp_vec,adative_range);
                adaptive_heart_normal = median(median_heart_vec,adative_range);
                adaptive_prdiff_normal = median(median_prdiff_vec,adative_range);
                adaptive_absmax_normal = median(median_absmax_vec,adative_range);
            }
        }
		//将上一个75点的呼吸率数据备份
        Data_transfer(X_X_1, X_X ,N_seg - N_seg_1, N_seg);
        Data_transfer(Pr_X_1, Pr_X ,N_seg - N_seg_1, N_seg);
        //将上一个750点的呼吸率心率数据备份
        Data_transfer(X_X_heart_1, X_X_heart , N_heart - N_heart_1, N_heart);
        //printf("Sleep after two minutes:\n");

        if(data_index > adative_range*N_seg){
        		current_period = AFTER_ADAPT;
        		if(current_period == AFTER_ADAPT && log_level < 1)
            	//printf("%d      %llu      %d      %d      %d  \n", current_period, data_index, read_raw1,read_raw2, read_raw3 );
            X_X_2[begin2] = read_raw1 - read_raw3; //压电
            Pr_X_2[begin2] = read_raw2; //压阻
            begin2++;
            if(begin2==N_seg - N_seg_1){
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
                //离床判断
                if(seg_pre_abssum < 200)
                {
                	printf("has left bed\n");
                	//做一些reset工作
                	datanum_after_inbed = 0;
                	//清理之前分配的内存
                	free(a2_resp);a2_resp = NULL;
									//free(a2_heart);a2_heart = NULL;
					finish = clock();
                	duration = (double)(finish - start) / CLOCKS_PER_SEC;
                	//printf( "\n算法时间：%f seconds\n", duration );
                	continue;
                }
                //计算呼吸频率
                resp_value = obtain_resprate_with_max_possibility(a2_resp,N_seg);
                //计算心率
                heart_value = obtain_heart_by_spec(X_X_heart,N_heart);
                //计算压阻一阶导数绝对值的和
                double * out_diff = (double *)malloc(N_seg*sizeof(double));

    						diff_fabs(Pr_X,N_seg,out_diff,&out_num);
    						pr_diff = onearray_sum(out_diff,out_num);
    						free(out_diff);out_diff = NULL;

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
                	make_further_polishment(pr_diff, absmax, adaptive_absmax_normal,
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
								//free(a2_heart);a2_heart = NULL;
                printf("\n%llusecond~",(data_index - adative_range*N_seg)/10-9);
                printf("%llusecond: ",(data_index - adative_range*N_seg)/10);

                printf("resp_value = %f,heart_value = %f\n",resp_value,heart_value);
                 HTTP_Body_Data[1]=(int)resp_value;
                 HTTP_Body_Data[2]=(int)heart_value;
                 http_data_type_flag=2;
                 xSemaphoreGive(adc_semaphore);
                finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC;
                printf( "\n算法时间：%f seconds\n", duration );
            }
        }         
    }
    free(median_energy_vec);median_energy_vec = NULL;
    free(median_resp_vec);median_resp_vec = NULL;
    free(median_heart_vec);median_heart_vec = NULL;
    free(median_prdiff_vec);median_prdiff_vec = NULL;
    free(median_absmax_vec);median_absmax_vec = NULL;

    free(X_X); X_X = NULL;
    free(Pr_X); Pr_X = NULL;
	free(X_X_heart); X_X_heart = NULL;
    free(X_X_1); X_X_1 = NULL;
    free(Pr_X_1); Pr_X_1 = NULL;
    free(X_X_2); X_X_2 = NULL;
    free(Pr_X_2); Pr_X_2 = NULL;
    free(X_X_heart_1); X_X_heart_1 = NULL;
    free(X_X_heart_2); X_X_heart_2 = NULL;
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
      initialise_wifi();//wifi链接
      adc_init();
     xTaskCreate(&adc1_get_data_task, "adc1_get_data_task", 8192, NULL, 5, NULL);
     //xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 8, NULL);
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

void make_further_polishment(double pr_diff, double absmax, double adaptive_absmax_normal,
														 double *p_resp_value, double *p_heart_value,
															int *p_resp_exception_counter, int *p_heart_exception_counter,
															history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos) {
	double history_resp_sum = 0.;
	double history_heart_sum = 0.;
	uint8_t field = -1;
	if(pr_diff < 1000) {
		 //======================呼吸率================================ [
		 //std函数比较费时间，暂时不用
		 //uint8_t condition_outliers = (absmax_vec(i) > 2*adaptive_absmax_normal || absmax_vec(i) - mean(X_seg) > 3*std(X_seg));
		 uint8_t condition_outliers = (absmax > 2*adaptive_absmax_normal );
     uint8_t whether_resp_abnormal = (*p_resp_value > 35 || *p_resp_value < 10);
     if(whether_resp_abnormal == 1) {//异常的呼吸率
            if(condition_outliers == 1) { //且存在异常点,放弃本次呼吸率，用过往历史
            		field = 3;
            		history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
                *p_resp_value = history_resp_sum/(front_pos - 0 + 1);
            }
            else { //没有异常点
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
            }
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
     		if(condition_outliers == 1) //且存在异常点, 放弃本次心率，用过往历史
     		{
     			field = 4;
          history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
          *p_heart_value = history_heart_sum/(front_pos - 0 + 1);
     		}
     		else  //没有异常点
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
     		}
     }
     if(whether_heart_abnormal == 0) //正常的心率,用当前和过往的做平均
     {
         field = 4;
         history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
         *p_heart_value = (history_heart_sum + *p_heart_value)/((front_pos - 0 + 1) + 1);
         *p_heart_exception_counter = 0;
     }
     // ========================================================= ]

	} else { //放弃本次呼吸率、心率，用过往历史 ??
		field = 3;//对应呼吸
		history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
    *p_resp_value = history_resp_sum/(front_pos - 0 + 1);
    *p_resp_exception_counter = 0;

    field = 4; //对应心率
    history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);
    *p_heart_value = history_heart_sum/(front_pos - 0 + 1);
     *p_heart_exception_counter = 0;
		printf("there is body movement\n");
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
