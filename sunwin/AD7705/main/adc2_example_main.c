#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "ad7705.h"

void app_main(void)
{   printf("初始化代码");
    unsigned int temp;
    //Initialize non-SPI GPIOs
    gpio_set_direction(SCLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(DIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(DOUT, GPIO_MODE_INPUT);
     gpio_set_direction(DRDY, GPIO_MODE_INPUT);
     AD7705_init1();
    temp= Read_datareg1();
    printf("%d",temp);
}

