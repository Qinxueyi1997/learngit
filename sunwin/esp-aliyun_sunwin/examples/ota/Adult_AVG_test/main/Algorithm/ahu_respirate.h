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

#ifndef __ESP_AHU_RESPIRATE_H__
#define __ESP_AHU_RESPIRATE_H__

#include <stdint.h>
#include <stdarg.h>
#include "sdkconfig.h"
#include <rom/ets_sys.h>

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
double obtain_resprate_by_spec(const double array[],int size);
double obtain_heart_by_spec(const double * sig,int sig_len);
void obtain_adaptive_filter_band(int * low_hight,const double * sig, int len,int fs);
int CalculateValidStartPoint(const double * indMax,const int * indexMax,const int max_len,
                             const double MAGNITUDE_THRSH,const double DISTANCE_MIN);
void ObtainValidPos(int * valid_pos,int *len1,const double * indMax,const int * indexMax,const double * indMin, const int * indexMin,
                    const int max_len,const int min_len,int start_pos,const double MAGNITUDE_THRSH,
                    const double DISTANCE_MIN);
void MergeAllValidPos(int *valid_pos, int *valid_pos_len,const int *omit_pos,const int * indexMax,
                      const int omit_len,const int max_len,const double DISTANCE_MIN);
int judge_whether_human(double * array,int size);
double obtain_resprate_by_corr(const double array[],int size);
double obtain_resprate_with_max_possibility(const double array[],int size);
int sleep_analysis(double * heart,int len,double norm_heart,double N_thr_value);
#ifdef __cplusplus
}
#endif


#endif /* __ESP_AHU_RESPIRATE_H__ */
