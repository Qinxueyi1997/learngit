#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_system.h"
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "platform/ssl_port.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"//esp_vfs_fat.h 头文件定义了连接 FatFs 和 VFS 的函数
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

static const char *TAG = "example";

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

 #define USE_SPI_MODE

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13
#endif //USE_SPI_MODE
int  adc[100]={4095,4095,4095,4095,4095,4095,4095,4095};
int pv1[10000]={4095,4095,4095,4095,4095,4095,4095,4095};
int pr1[10000]={4095,4095,4095,4095,4095,4095,4095,4095};
char pv[10],pr[10],flag[10],write_flag=0,start_flag=0;
char buf[100]={"4095"};
unsigned long int over_flag=0;										
void app_main(void)
{
  
    ESP_LOGI(TAG, "Initializing SD card");
#ifndef USE_SPI_MODE
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

#else
    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
#endif //USE_SPI_MODE

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {//挂载sd卡　
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 2 * 1024
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
       // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/sdcard/adcdata.csv", "a");
    
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    uint32_t num1=0,num2=0,num3=0;
    int read_raw1, read_raw2, read_raw3,read_raw11;
  
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



    while(1) {
    
      
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
        read_raw1 = adc1_get_raw( ADC1_CHANNEL_6);//压电
        read_raw2 = adc1_get_raw( ADC1_CHANNEL_7);//压阻
        read_raw3 = adc1_get_raw( ADC1_CHANNEL_5);//VREF
        over_flag++;
        read_raw11=read_raw1-read_raw3;
       itoa(read_raw11,pv,10);
       itoa(read_raw2,pr,10);
       itoa(over_flag,flag,10);
       fprintf(f, " NUM, %s, PV ,%s, PR , %s\n",flag, pv,pr);
       fflush(f);
 
           if(over_flag==2000){
           
                  fclose(f);
                  ESP_LOGI(TAG, "File written");
                  esp_vfs_fat_sdmmc_unmount();
                  ESP_LOGI(TAG, "Card unmounted");
           }

          
  
           

    }





}
