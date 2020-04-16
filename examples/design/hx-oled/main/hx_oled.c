/*
* @file         hx_oled.c 
* @brief        ESP32操作OLED-I2C
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       秦学义
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
*/
#include <stdio.h>
#include "esp_system.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "oled.h"
#include "fonts.h"
char rec_buf[4]={123456}; 
uint32_t rec_data=23;
void app_main()
{
    
    oled_init();
   oled_show_str(0,0,  "Made by Qinxueyi", &Font_7x10, 1);
    oled_show_str(0,15, "TEMP :", &Font_7x10, 1);
    oled_show_str(0,30, "HUM  :" , &Font_7x10, 1);
    oled_show_str(0,45, "PM2.5:" , &Font_7x10, 1);

     oled_show_str(80,15,"%c", &Font_7x10, 1);
    oled_show_str(80,30,"%rh", &Font_7x10, 1);
    oled_show_str(80,45,"%g/m3" , &Font_7x10, 1);

    oled_show_num(50,15, rec_data, &Font_7x10, 1);
    oled_show_num(50,30, rec_data, &Font_7x10, 1);
    oled_show_num(50,45, rec_data, &Font_7x10, 1);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    while(1)
    {
       // cnt++;
       // oled_claer();
       // vTaskDelay(10 / portTICK_PERIOD_MS);
       // oled_all_on();
      //  vTaskDelay(10 / portTICK_PERIOD_MS);
      //  ESP_LOGI("OLED", "cnt = %d \r\n", cnt);
    }
}