/** 
* @file         user_app_main.c 
* @brief        用户应用程序入口
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
*/

/* =============
头文件包含
 =============*/
#include <stdio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "driver/adc.h"
#include "esp_event.h"


/** 
 * wifi事件处理函数
 * @param[in]   ctx     :表示传入的事件类型对应所携带的参数
 * @param[in]   event   :表示传入的事件类型
 * @retval      ESP_OK  : succeed
 *              其他    :失败 
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_START:
        printf("\nwifi_softap_start\n");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        printf("\nwifi_softap_connectted\n");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        printf("\nwifi_softap_disconnectted\n");
        break;
    default:
        break;
    }
    return ESP_OK;
}

/** 
 * 应用程序的函数入口
 * @param[in]   NULL
 * @retval      NULL              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
 */
void app_main()
{    
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "智能护理床",
            .ssid_len = 0,
            /* 最多只能被4个station同时连接,这里设置为只能被一个station连接 */
            .max_connection = 1,
            .password = "12345678",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
     //ESP_ERROR_CHECK(esp_wifi_start());    

    uint8_t output_data=0;
    uint8_t time=0;
    uint32_t num1=0,num2=0,num3=0;
    int     read_raw1, read_raw2, read_raw3;
    esp_err_t r,r1,r2,r3;

    gpio_num_t adc_gpio_num1,adc_gpio_num2, adc_gpio_num3;

    r = adc1_pad_get_io_num( ADC1_CHANNEL_5, &adc_gpio_num1 );
    assert( r == ESP_OK );
     r = adc1_pad_get_io_num( ADC1_CHANNEL_6, &adc_gpio_num2 );
    assert( r == ESP_OK );
     r = adc1_pad_get_io_num( ADC1_CHANNEL_7, &adc_gpio_num3 );
    assert( r == ESP_OK );

    //be sure to do the init before using adc2. 
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_5, ADC_ATTEN_11db );
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_6, ADC_ATTEN_11db );
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten( ADC1_CHANNEL_7, ADC_ATTEN_11db );
    vTaskDelay(2 * portTICK_PERIOD_MS);

    printf("start conversion.\n");
    while(1) {

       
         vTaskDelay(18/ portTICK_PERIOD_MS);
        time++;
        num1+= adc1_get_raw( ADC1_CHANNEL_5);//压电
        num2+= adc1_get_raw( ADC1_CHANNEL_6);//压阻
        num3+= adc1_get_raw( ADC1_CHANNEL_7);//VREF基准
        if ( time==10 ) {
            output_data++; 
            time=0;
            read_raw1=num1/10;
            read_raw2=num2/10;
            read_raw3=num3/10;
            num1=0;num2=0;num3=0;
            printf("%d:      PV:  %d      PR:  %d      VREF:  %d  \n", output_data, read_raw3,read_raw2, read_raw1);
        } 


        
    }
}
