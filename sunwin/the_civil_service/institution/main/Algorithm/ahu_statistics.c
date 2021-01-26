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
#include "esp_err.h"
#include "esp_attr.h"
#include "rom/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
double median(const double array[],int size)
{
    double  * array1 = (double *)malloc(size*sizeof(double));
    double temp;
    double mid;
    uint8_t flag = 1;
    
    assert(size > 2);
    
    for(int i = 0; i < size; i++)
    {
            array1[i] = array[i];
    }
    //冒锟斤拷
    for (int i = 1; i < size && flag == 1; i++)
    { 
    	flag = 0;
      for(int j = 0;j < size - i; j++)
        if (array1[j] > array1[j+1]) 
        { 
        	flag = 1;
          temp = array1[j]; array1[j] = array1[j+1]; array1[j+1] = temp;
        }
    }
   
    if(size % 2 == 0){
        mid = (array1[size/2] + array1[size/2-1])/2.;
    }
    else{
        mid = array1[(size-1)/2];
    }
    free(array1);
    return mid;
}
