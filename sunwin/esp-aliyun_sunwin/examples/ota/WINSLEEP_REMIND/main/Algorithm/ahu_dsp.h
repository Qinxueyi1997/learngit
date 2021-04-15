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

#ifndef __ESP_AHU_DSP_H__
#define __ESP_AHU_DSP_H__

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
void dft(double * pr,double * pi,int n,double * fr,double * fi);
void idft(double * fr,double * fi,int n,double * pr,double * pi);
void filter(const double* x, double* y, int xlen, double* a, double* b, int nfilt);
void hilbert(double * sig, double * hilt_sig, int len);
int fft(float * pr,float * pi,int len,float * fr,float * fi);
int ifft(float * fr,float * fi,int len,float * pr,float * pi);
#ifdef __cplusplus
}
#endif


#endif /* __ESP_AHU_DSP_H__ */
