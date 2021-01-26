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
#include <malloc.h>
#include "esp_err.h"
#include "esp_attr.h"
#include "rom/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ahu_utils.h"

#define LOG_LOCAL_LEVEL CONFIG_LOG_DEFAULT_LEVEL
void Data_transfer(double array1[],const double array2[],int size1,int size2)
{
    for(int i = size1; i < size2; i++){
        array1[i-size1] = array2[i];
    }
}
void sort_descend(double array[],int size) //闂佽法鍠愮敮瀛樻綇閻愵剙顏堕悘蹇撶箻閳ь剙顦甸弫鎾诲棘閵堝棗顏堕梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾诲棘閵堝棗顏�
{
    double temp;//闂佽法鍠愰弸濠氬箯闁垮顦ч梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氾拷
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] > array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
void sort_ascend(double array[],int size) //闂佽法鍠愰弸濠氬箯瀹勬壆姣堥梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氬綊鏌呮径鎰櫢闁哄倶鍊栫€氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾诲棘閵堝棗顏堕梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氾拷
{
    double temp;//闂佽法鍠愰弸濠氬箯闁垮顦ч梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氾拷
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] < array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
void sort(int array[],int size)
{
    int temp;//闂佽法鍠愰弸濠氬箯闁垮顦ч梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁哄倶鍊栫€氾拷
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] < array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
int ismember(const int array[],const int size,int e)
{
    for(int i = 0; i < size; i++){
        if(array[i] == e)
            return 1;
    }
    return 0;
}
double mean(const double array[],int length1, int length2)
{
    double sum = 0;
    for(int i = length1; i < length2; i++)
    {
        sum += array[i];
    }
    double m = sum/(length2-length1);
    return m;
}
double standard_deviation(const double array[],int len1,int len2){
    double avg = mean(array,len1,len2);
    double sum = 0.;
    for(int i = len1; i < len2; i++){
        sum += (array[i] - avg) * (array[i] - avg);
    }
    return sqrt(sum/(len2-len1));
}
void max_v_interval(const double array[],double maxValueIdex[],int size)
{
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < array[i]){
            max_value = array[i];
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index + 1;
}

void max_v_interval_fabs(const double array[],double maxValueIdex[],int size)
{
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < fabs(array[i])){
            max_value = fabs(array[i]);
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index;
}

void max_v_spec(const double array[],double maxValueIdex[],int size)
{
    double * temp = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++)
    {
        temp[i] = array[i+1];
    }
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < temp[i]){
            max_value = temp[i];
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index + 1;
    free(temp);temp = NULL;
}
double onearray_sum(double array[],int size)
{
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum = sum + array[i];
    }
    return sum;
}
double onearray_sum_fabs(double array[],int size)
{
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum = sum + fabs(array[i]);
    }
    sum=sum/size;
    return sum;
}
//matlab闂佽法鍠愰弸濠氬箯缁屽ff闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢鍛婄伄闁瑰嚖鎷�
void diff(double *dat,int datalen,double*ret_buf,int *ret_len)
{
 	int i,j=0;
  for(i = 1; i<datalen; i++)
  {
    ret_buf[j++] = dat[i]-dat[i-1];
  }
  *ret_len = j;
}

void diff_fabs(double *dat,int datalen,double*ret_buf,int *ret_len)
{
 	int i,j=0;
  for(i = 1; i<datalen; i++)
  {
    ret_buf[j++] = fabs(dat[i]-dat[i-1]);
  }
  *ret_len = j;
}

double accumulate1(const double array[], int size,int seg)
{
    double * mul = (double *)malloc(size*sizeof(double));
    double * mul_divide = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++){
        mul[i] = array[i]*array[i];
    }
    for(int i = 0; i < size; i++){
        mul_divide[i] = mul[i]/seg;
    }
    double sum = onearray_sum(mul_divide,size);
    double energy_vec = sqrt(sum);
    free(mul); mul = NULL;
    free(mul_divide);mul_divide = NULL;
    return energy_vec;
}
double accumulate2(const double array1[], const double array2[],int size)
{
    double * mul1 = (double *)malloc(size*sizeof(double));
    double * mul2 = (double *)malloc(size*sizeof(double));
    double * mul3 = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++){
        mul1[i] = array1[i]*array2[i];
    }
    for(int i = 0; i < size; i++){
        mul2[i] = array1[i]*array1[i];
    }
    for(int i = 0; i < size; i++){
        mul3[i] = array2[i]*array2[i];
    }
    double sum1 = onearray_sum(mul1,size);
    double sum2 = onearray_sum(mul2,size);
    double sum3 = onearray_sum(mul3,size);

    double result = sum1/sqrt(sum2*sum3);
    free(mul1);mul1 = NULL;
    free(mul2);mul2 = NULL;
    free(mul3);mul3 = NULL;
    return result;
}

/** 闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢鐑樺箰eakdet闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢鍛婄伄闁瑰嘲鍢查埀顒€銈搁弫鎾诲棘閵堝棗顏舵慨锝呯箻閺佹捇寮妶鍡楊伓闂佽法鍠愰弸濠氬箯瀹勬壋鍋撴總鍛婃櫢闁哄倶鍊栫€氳鎯旈弮鍫熸櫢闁哄倶鍊栫€氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾诲棘閵堝棗顏堕梺璺ㄥ枙椤鏁钘夘伓c闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢鑽ょ獩閻㈩垼鍠楃€氬綊骞楀ú顏呮櫢闁哄倶鍊栫€氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾诲棘閵堝棗顏堕悷鏇氱窔閺佹捇鎯岄銈庢殰闁归鍏橀弫鎾诲棘閵堝棗顏堕柛濠傘偢閺佹捇寮妶鍡楊伓闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢绋跨枂閻㈩垼鍠楃€氱atlab濞戞挴鍋撻梺璺ㄥ枛閹筋亪鏁愯箛鏂款伓闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柣顓у亞椤ｎ噣骞忛柨瀣槯闂佽法鍠愰弸濠氬箯闁垮鏆堥梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁跨噦鎷�1*/
void peakdet(const double * num,int N,double * indMax,int * indexMax,double * indMin,
             int * indexMin,int * max_len,int * min_len,double delta)
{
    double mn = 99999999., mx = -99999999.;
    int mnpos = 0, mxpos = 0;
    int lookformax = 1;
    double thi;
    //int a = 0;
    for(int i = 0;i < N;i++)
    {
        thi = num[i];
        if(thi > mx){
            mx = thi;
            mxpos = i;
        }
        if(thi < mn){
            mn = thi;
            mnpos = i;
        }

        if(lookformax){
            if(thi <  mx-delta){
                indexMax[*max_len] = mxpos;
                indMax[*max_len] = num[mxpos];
                (*max_len)++;
                mn = thi;
                mnpos = i;
                lookformax = 0;
            }
        }
        else{
            if(thi > mn+delta){
                indexMin[*min_len] = mnpos;
                indMin[*min_len] = num[mnpos];
                (*min_len)++;
                mx = thi;
                mxpos = i;
                lookformax = 1;
            }

        }
    }
}
void detect_local_interval_by_max(const double array[],double a[],int size,int Nmin,int Nmax)
{
    int center_p,temp;
    temp = Nmax-Nmin+1;
    center_p = size/2 - 1;
    int center = center_p +1;
    double * corr_vec = (double *)malloc(size*sizeof(double));

    for(int i = Nmin; i <= Nmax; i++)
    {
        double * seg_left = (double *)malloc(i*sizeof(double));
        double * seg_right = (double *)malloc(i*sizeof(double));
        int begin1 = center_p-i+1;
        int end1 = center_p+i;
        Data_transfer(seg_left,array, begin1, center);
        Data_transfer(seg_right,array, center_p, end1);
        corr_vec[i-Nmin] = accumulate2(seg_left, seg_right,i);
        free(seg_left);seg_left = NULL;
        free(seg_right);seg_right = NULL;
    }
    double max_value_index[2];
    max_v_interval(corr_vec,max_value_index, temp);
    free(corr_vec);corr_vec = NULL;
    double max_value = max_value_index[0];
    int max_index = max_value_index[1];
    int isend = 0;
    if(max_index == 1 || max_index == Nmax-Nmin+1){
        isend = 1;
    }
    max_index = Nmin+max_index - 1;
    a[0] = max_index;
    a[1] = max_value;
    a[2] = isend;
}
void detect_local_interval_by_peakdet(const double array[],double a[],int size,int Nmin,int Nmax)
{
    int center_p,temp;
    temp = Nmax-Nmin+1;
    center_p = size/2 - 1;
    int center = center_p +1;
    double * corr_vec = (double *)malloc(temp*sizeof(double));
    for(int i = Nmin; i <= Nmax; i++)
    {
        double * seg_left = (double *)malloc(i*sizeof(double));
        double * seg_right = (double *)malloc(i*sizeof(double));
        int begin1 = center_p-i+1;
        int end1 = center_p+i;
        Data_transfer(seg_left,array, begin1, center);
        Data_transfer(seg_right,array, center_p, end1);
        corr_vec[i-Nmin] = accumulate2(seg_left, seg_right,i);
        free(seg_left);seg_left = NULL;
        free(seg_right);seg_right = NULL;
    }
    double max_value_index[2];
    max_value_index[0] = 1.;
    max_value_index[1] = -1.;
    double * indMax = (double *)malloc(50*sizeof(double));
    int * indexMax = (int *)malloc(50*sizeof(int));
    double * indMin = (double *)malloc(50*sizeof(double));
    int * indexMin = (int *)malloc(50*sizeof(int));
    int max_len=0,min_len=0;
    double delta = 0.01;
    peakdet(corr_vec,temp,indMax,indexMax,indMin,indexMin,&max_len,&min_len,delta);
    if(max_len == 0){
        a[0] = max_value_index[0];
        a[1] = max_value_index[1];
        a[2] = 1;
        free(indMax);indMax = NULL;
        free(indexMax);indexMax = NULL;
        free(indMin);indMin = NULL;
        free(indexMin);indexMin = NULL;
		free(corr_vec); corr_vec = NULL;
        return;
    }
    for(int i = 0; i < max_len; i++){
        int offset_pos = indexMax[i] + 1 + Nmin - 1;
        if(offset_pos > 17 && offset_pos < 62){
            if(indMax[i] > max_value_index[1]){
                max_value_index[0] = indexMax[i] + 1;
                max_value_index[1] = indMax[i];
            }
        }
    }
    free(indMax);indMax = NULL;
    free(indexMax);indexMax = NULL;
    free(indMin);indMin = NULL;
    free(indexMin);indexMin = NULL;
    free(corr_vec); corr_vec = NULL;
    int isend = 0;
    if(max_value_index[0] == 1 || max_value_index[0] == Nmax-Nmin+1){
        isend = 1;
    }
    max_value_index[0] = Nmin + max_value_index[0] - 1;
    a[0] = max_value_index[0];
    a[1] = max_value_index[1];
    a[2] = isend;
}

void save_history_info(history_statistics his_stat_arr[], int arr_size,int *his_stat_arr_top, history_statistics cur_stat)
{
		assert(*his_stat_arr_top < arr_size && *his_stat_arr_top >= 0);

		his_stat_arr[*his_stat_arr_top].absmax = cur_stat.absmax;
		his_stat_arr[*his_stat_arr_top].seg_energy = cur_stat.seg_energy;
		his_stat_arr[*his_stat_arr_top].pr_diff = cur_stat.pr_diff;
		his_stat_arr[*his_stat_arr_top].resp_value = cur_stat.resp_value;
		his_stat_arr[*his_stat_arr_top].heart_value = cur_stat.heart_value;

		*his_stat_arr_top = (*his_stat_arr_top + 1)%arr_size;
}

double  get_history_info_sum_of_some_field(history_statistics his_stat_arr[], int arr_size, int begin, int end, uint8_t field)
{
		int i = 0;
		double fsum = 0.;
		assert(begin <= arr_size && begin >=0);
		assert(end <= arr_size && end >=0);
		assert(begin < end);
		assert(field < 5);
		for(i = begin; i < end;i++)
		{
			switch(field)
			{
				case 0:
					fsum += his_stat_arr[i].absmax;
					break;
				case 1:
					fsum += his_stat_arr[i].seg_energy;
					break;
				case 2:
					fsum += his_stat_arr[i].pr_diff;
					break;
				case 3:
					fsum += his_stat_arr[i].resp_value;
					break;
				case 4:
					fsum += his_stat_arr[i].heart_value;
					break;
				default:
					break;
			}
		}
		return fsum;
}
