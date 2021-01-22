// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This module implements pthread API on top of FreeRTOS. API is implemented to the level allowing
// libstdcxx threading framework to operate correctly. So not all original pthread routines are supported.
//


#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "esp_err.h"
#include "esp_attr.h"
#include "rom/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ahu_dsp.h"
#include "sw_al.h"
#define EPS 0.000001
double linear_regression(double *array_x,double *array_y,int size_t)
{
double a=0,b=0,sum_x=0,sum_y=0,x_ave=0,y_ave=0,lxy=0,xisub=0;;
for(int i=0;i<size_t;i++)
{
	sum_x+=array_x[i];
    	sum_y+=array_y[i];
}
x_ave=sum_x/size_t;
y_ave=sum_y/size_t;
for(int i=0;i !=size_t;i++)
{
lxy+=(array_x[i]-x_ave)*(array_y[i]-y_ave);
xisub+=(array_x[i]-x_ave)*(array_x[i]-x_ave);
}
b=lxy/xisub;
a=y_ave-b*x_ave;
printf("y=%0.2fx+%0.2f\n", b, a);
return b;
}

