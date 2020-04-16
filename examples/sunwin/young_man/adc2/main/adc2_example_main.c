/* ADC2 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_system.h"

#define DAC_EXAMPLE_CHANNEL     CONFIG_EXAMPLE_DAC_CHANNEL
#define ADC2_EXAMPLE_CHANNEL    CONFIG_EXAMPLE_ADC2_CHANNEL//模拟转数字
 
void app_main(void)
{
    uint8_t output_data=0;
    uint8_t time=0;
    uint32_t num1=0,num2=0,num3=0;
    int     read_raw1, read_raw2, read_raw3;
    esp_err_t r,r1,r2,r3;

    gpio_num_t adc_gpio_num1,adc_gpio_num2, adc_gpio_num3;

    r = adc2_pad_get_io_num( ADC2_CHANNEL_5, &adc_gpio_num1 );
    assert( r == ESP_OK );
     r = adc2_pad_get_io_num( ADC2_CHANNEL_6, &adc_gpio_num2 );
    assert( r == ESP_OK );
     r = adc2_pad_get_io_num( ADC2_CHANNEL_7, &adc_gpio_num3 );
    assert( r == ESP_OK );

    //be sure to do the init before using adc2. 
   
    adc2_config_channel_atten( ADC2_CHANNEL_5, ADC_ATTEN_11db );
    adc2_config_channel_atten( ADC2_CHANNEL_6, ADC_ATTEN_11db );
    adc2_config_channel_atten( ADC2_CHANNEL_7, ADC_ATTEN_11db );
    vTaskDelay(2 * portTICK_PERIOD_MS);

    printf("start conversion.\n");
    while(1) {

       // vTaskDelay(32/ portTICK_PERIOD_MS);
        //vTaskDelay( 2 * portTICK_PERIOD_MS );
        // usleep(1250);
       //ets_delay_us(1250);
         vTaskDelay(18/10/ portTICK_PERIOD_MS);
        time++;
        r1 = adc2_get_raw( ADC2_CHANNEL_5, ADC_WIDTH_12Bit, &read_raw1);//压电
        r2 = adc2_get_raw( ADC2_CHANNEL_6, ADC_WIDTH_12Bit, &read_raw2);//压阻
        r3 = adc2_get_raw( ADC2_CHANNEL_7, ADC_WIDTH_12Bit, &read_raw3);//VREF基准
       num1=num1+read_raw1;
       num2=num2+read_raw2;
       num3=num3+read_raw3;

        if ( (r1 == ESP_OK)&&(r2 == ESP_OK)&&(r3 == ESP_OK)&&(time==10) ) {
            output_data++; 
            time=0;
            read_raw1=num1/10;
            read_raw2=num2/10;
            read_raw3=num3/10;
            num1=0;num2=0;num3=0;
            printf("%d:      PV:  %d      PR:  %d      VREF:  %d  \n", output_data, read_raw1,read_raw2, read_raw3 );
        } else if ( r1== ESP_ERR_INVALID_STATE ) {
            //printf("%s: ADC2 not initialized yet.\n", esp_err_to_name(r1));
        } else if ( r1 == ESP_ERR_TIMEOUT ) {
            //This can not happen in this example. But if WiFi is in use, such error code could be returned.
         //   printf("%s: ADC2 is in use by Wi-Fi.\n", esp_err_to_name(r1));
        } else {
          //  printf("%s\n", esp_err_to_name(r1));
        }

       
        
    }
}

