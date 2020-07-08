// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#ifndef __ESP_AHU_UTILS_H__
#define __ESP_AHU_UTILS_H__

#include <stdint.h>
#include <stdarg.h>
#include "sdkconfig.h"
#include <rom/ets_sys.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set log level for given tag
 *
 * If logging for given component has already been enabled, changes previous setting.
 *
 * Note that this function can not raise log level above the level set using
 * CONFIG_LOG_DEFAULT_LEVEL setting in menuconfig.
 *
 * To raise log level above the default one for a given file, define
 * LOG_LOCAL_LEVEL to one of the ESP_LOG_* values, before including
 * esp_log.h in this file.
 *
 * @param tag Tag of the log entries to enable. Must be a non-NULL zero terminated string.
 *            Value "*" resets log level for all tags to the given value.
 *
 * @param level  Selects log level to enable. Only logs at this and lower verbosity
 * levels will be shown.
 */
//������Ź�ȥ2��10���ڣ�1���źŵ����ֵ��2���źŵ�����ֵ��3��ѹ��Ĳ���ֵ��4�������ʣ�5������
typedef struct 
{
	double absmax;
	double seg_energy;
	double pr_diff;
	double resp_value;
	double heart_value;        
} history_statistics;
void save_history_info(history_statistics his_stat_arr[], int arr_size,int *his_stat_arr_top, history_statistics cur_stat);	
double  get_history_info_sum_of_some_field(history_statistics his_stat_arr[], int arr_size, int begin, int end, uint8_t field); 
void diff(double *dat,int datalen,double*ret_buf,int *ret_len);
void diff_fabs(double *dat,int datalen,double*ret_buf,int *ret_len);
double onearray_sum(double array[],int size);
double onearray_sum_fabs(double array[],int size);
void Data_transfer(double array1[],const double array2[],int size1,int size2);
void max_v_interval(const double array[],double maxValueIdex[],int size);
void max_v_interval_fabs(const double array[],double maxValueIdex[],int size);
void max_v_spec(const double array[],double maxValueIdex[],int size);
double accumulate1(const double array[], int size,int seg);
double accumulate2(const double array1[], const double array2[],int size);
void detect_local_interval_by_max(const double array[],double a[],int size,int Nmin,int Nmax);
void detect_local_interval_by_peakdet(const double array[],double a[],int size,int Nmin,int Nmax);
void peakdet(const double * num,int N,double * indMax,int * indexMax,double * indMin, int * indexMin,int * max_len,int * min_len,double delta);
#ifdef __cplusplus
}
#endif


#endif /* __ESP_AHU_UTILS_H__ */
