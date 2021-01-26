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
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"
#include "butter_coefficient.h"
int Fs = 10;
static void dft_abs(double * fr,double * fi,int n,double * out)
{
    for (int i=0; i< n; i++)
    {
        out[i]=sqrt(fr[i]*fr[i]+fi[i]*fi[i]);  //幅值计算
    }
}
static void obtain_impulse_heart_sig(const double * input, double * output, int len, int f_len)
{

    //double * f0 = (double *)malloc(f_len*sizeof(double));
    //double * f1 = (double *)malloc(f_len*sizeof(double));
    //double * f2 = (double *)malloc(f_len*sizeof(double));
    //double * f3 = (double *)malloc(f_len*sizeof(double));
    //double * f_1 =(double *)malloc(f_len*sizeof(double));
    //double * f_2 = (double *)malloc(f_len*sizeof(double));
    //double * f_3 = (double *)malloc(f_len*sizeof(double));
    //int len0=len-3,len1=len-2,len2=len-1,len3=len,len_1=len-4,len_2=len-5,len_3=len-6;
    //Data_transfer(f0, input, 3, len0);
    //Data_transfer(f1, input, 4, len1);
    //Data_transfer(f2, input, 5, len2);
    //Data_transfer(f3, input, 6, len3);
    //Data_transfer(f_1, input, 2, len_1);
    //Data_transfer(f_2, input, 1, len_2);
    //Data_transfer(f_3, input, 0, len_3);
    //for(int i = 0; i < f_len; i++){
        //output[i] = 4*f0[i] + (f1[i]+f_1[i]) - 2*(f2[i]+f_2[i]) - (f3[i]+f_3[i]);
    //}
	for(int i = 0; i < f_len; i++){
        output[i] = 4*input[i+3] + (input[i+4]+input[i+2]) - 2*(input[i+5]+input[i+1]) - (input[i+6]+input[i]);
    }
    //free(f0);f0 = NULL;
    //free(f1);f1 = NULL;
    //free(f2);f2 = NULL;
    //free(f3);f3 = NULL;
    //free(f_1);f_1 = NULL;
    //free(f_2);f_2 = NULL;
    //free(f_3);f_3 = NULL;
}

void obtain_adaptive_filter_band(int * low_hight,const double * sig, int len,int fs)
{
    int k = (int)ceil((double)log2(len));
    int n = pow(2,k);
	float * pr  = (float *)malloc(n*sizeof(float));
    float * pi = (float *)malloc(n*sizeof(float));
    float * fr = (float *)malloc(n*sizeof(float));
    float * fi = (float *)malloc(n*sizeof(float));
    for (int i=0; i < len; i++)  //生成输入信号
    {
			pr[i]=sig[i];
			pi[i]=0.0;
    }
    n = fft(pr,pi,len,fr,fi);
    int half_len = n/2;
    double * fs_vec = (double *)malloc(half_len*sizeof(double));
    for(int i = 1; i <= half_len; i++){
        fs_vec[i-1] = (double)fs*i/n;
    }
    double * mag_vec = (double *)malloc(half_len*sizeof(double));
    for(int i = 0; i < half_len; i++){
        mag_vec[i] = sqrt(fr[i]*fr[i]+fi[i]*fi[i]);
    }
    free(pr);pr = NULL;
    free(pi);pi = NULL;
    free(fr);fr = NULL;
    free(fi);fi = NULL;
    free(fs_vec); fs_vec = NULL;
    int windowSize = 10;
    double * b = (double *)malloc(windowSize*sizeof(double));
    for(int i = 0; i < windowSize; i++){
        b[i] = 1.0/windowSize;
    }
    double * a = (double *)malloc(windowSize*sizeof(double));
    a[0] = 1.0;
    for(int i = 1; i < windowSize; i++){
        a[i] = 0;
    }
    double * temp1 = (double *)malloc(half_len*sizeof(double));
    filter(mag_vec,temp1,half_len,a,b,windowSize);
    free(b); b = NULL;
    free(a); a = NULL;
    free(mag_vec); mag_vec = NULL;
    double * indMax = (double *)malloc(200*sizeof(double));
    int * indexMax = (int *)malloc(200*sizeof(int));
    double * indMin = (double *)malloc(200*sizeof(double));
    int * indexMin = (int *)malloc(200*sizeof(int));
    int max_len=0,min_len=0;
    peakdet(temp1,half_len,indMax,indexMax,indMin,indexMin,&max_len,&min_len,500.0);
	free(temp1); temp1 = NULL;
    if(max_len == 0){
        low_hight[0] = -1;
        low_hight[1] = -1;
		free(indMin);indMin = NULL;
		free(indexMin);indexMin = NULL;
		free(indMax);indMax = NULL;
		free(indexMax);indexMax = NULL;
        return;
    }
    for(int i = 0; i < max_len; i++){
        indexMax[i] = indexMax[i] +1;     //加1使其与matlab结果保持一致
    }
    int start_pos = 60;
    double * valid_max_pos = (double *)malloc(max_len*sizeof(double));
    double * valid_max_value = (double *)malloc(max_len*sizeof(double));
    int valid_len = 0;
    for(int i = 0; i < max_len; i++){
        if(indexMax[i] > start_pos){
            valid_max_pos[valid_len] = indexMax[i];
            valid_max_value[valid_len] = indMax[i];
            valid_len++;
        }
    }
    double maxValueIdex[2];
    max_v_interval(valid_max_value,maxValueIdex,valid_len);
    double a1 = maxValueIdex[0];
    int b1 = valid_max_pos[(int)maxValueIdex[1]-1];//按matlab习惯在max_v_interval里加了1，因此这里要对应减去1.
    low_hight[0] = fs*b1/n -2;
    low_hight[1] = low_hight[0] +4;
    if(low_hight[0]<4 || low_hight[0]>20){
        low_hight[0] = -1;
    }
    free(indMin);indMin = NULL;
    free(indexMin);indexMin = NULL;
    free(indMax);indMax = NULL;
    free(indexMax);indexMax = NULL;
    free(valid_max_pos);valid_max_pos = NULL;
    free(valid_max_value);valid_max_value = NULL;

}
int CalculateValidStartPoint(const double * indMax,const int * indexMax,const int max_len,
                             const double MAGNITUDE_THRSH,const double DISTANCE_MIN)
{
    int start_pos = -1;
    for(int i = 0; i < max_len; i++){
        int not_start_end = 0;
        if(indexMax[i] > 5){
            not_start_end = 1;
        }
        int not_small_value = 0;
        if(indMax[i] > MAGNITUDE_THRSH){
            not_small_value = 1;
        }
        if(not_start_end && not_small_value){
            start_pos = i;
            //增加一个有效性判断，附近是否有一个更为合适的起始点
            double next_max_value = -1;
            if(start_pos+1 < max_len){
                next_max_value = indMax[start_pos+1];
            }
            double current_max_value = indMax[start_pos];
            double high_ratio = (next_max_value-current_max_value)/current_max_value;
            if(high_ratio > 0.75){
                if(indexMax[i+1]-indexMax[i] < DISTANCE_MIN){
                    start_pos = i + 1;
                    i = i +1;
                }
            }
            break;
        }
    }
    return start_pos;
}

void ObtainValidPos(int * valid_pos,int *len1,const double * indMax,const int * indexMax,const double * indMin, const int * indexMin,
                    const int max_len,const int min_len,int start_pos,const double MAGNITUDE_THRSH,
                    const double DISTANCE_MIN)
{
    valid_pos[0] = start_pos;
    int last_pos = start_pos;
    *len1 = 1;
    for(int i = start_pos+1; i < max_len; i++){
        int valid_distance = 0;
        if((indexMax[i]-indexMax[last_pos]) > DISTANCE_MIN){
            valid_distance = 1;
        }
        int valid_magnitude = 0;
        if(indMax[i] > MAGNITUDE_THRSH){
            valid_magnitude = 1;
        }
        //检查附近是否有一个降幅很小的极小值点，如果有，放弃
        int valid_wave = 1;
        int min_range_left = indexMax[i] - 10;
        int min_range_right = indexMax[i] + 10;
        for(int jj = 0; jj < min_len; jj++){
            if(indexMin[jj]>min_range_left && indexMin[jj]<min_range_right){
                if(indMax[jj]-indMin[jj] < 10){
                    valid_wave = 0;
                    break;
                }
            }
        }
        if(valid_wave == 0)
            continue;
        if(valid_distance && valid_magnitude){
            last_pos = i;
            valid_pos[(*len1)++] = i;
        }
    }
}

void MergeAllValidPos(int *valid_pos, int *valid_pos_len,const int *omit_pos,const int * indexMax,
                      const int omit_len,const int max_len,const double DISTANCE_MIN)
{
    int * valid_omit = (int *)malloc(omit_len*sizeof(int));
    int all_valid_len = *valid_pos_len + omit_len;
    int * all_valid = (int *)malloc(all_valid_len*sizeof(int));
    for(int i = 0;i < all_valid_len; i++){
        if(i < *valid_pos_len){
            all_valid[i] = valid_pos[i];
        }else{
            all_valid[i] = omit_pos[i-(*valid_pos_len)];
        }
    }
    sort(all_valid,all_valid_len);
    int a1 = 0;
    for(int i = 0;i < all_valid_len; i++){
        if(ismember(omit_pos,omit_len,all_valid[i])){
            if(indexMax[all_valid[i]] < indexMax[valid_pos[0]] && indexMax[valid_pos[0]] - indexMax[all_valid[i]] > DISTANCE_MIN){
                valid_omit[a1] = all_valid[i];
                a1++;
                continue;
            }
            if(indexMax[all_valid[i]] > indexMax[valid_pos[0]] && indexMax[all_valid[i]] < indexMax[valid_pos[*valid_pos_len-1]]){
                if(indexMax[all_valid[i]] - indexMax[all_valid[i-1]] > DISTANCE_MIN && indexMax[all_valid[i+1]] - indexMax[all_valid[i]] > DISTANCE_MIN){
                    valid_omit[a1] = all_valid[i];
                    a1++;
                    continue;
                }
            }
            if(indexMax[all_valid[i]] > indexMax[valid_pos[*valid_pos_len-1]] && indexMax[all_valid[i]] - indexMax[valid_pos[*valid_pos_len-1]] > DISTANCE_MIN){
                valid_omit[a1] = all_valid[i];
                a1++;
                continue;
            }
        }
    }
    *valid_pos_len = *valid_pos_len + a1;
    for(int i = *valid_pos_len-a1;i < *valid_pos_len; i++){
        valid_pos[i] = valid_omit[i-(*valid_pos_len-a1)];
    }
    sort(valid_pos,*valid_pos_len);
    free(valid_omit); valid_omit = NULL;
    free(all_valid); all_valid = NULL;
}

int judge_whether_human(double * array,int size)
{
    int is_not_human = 0;
    double * X_seg_demean = (double *)malloc(size*sizeof(double));
    for(int i = 0; i <size ;i++){
        X_seg_demean[i] = array[i] - mean(array,0,size);
        if(X_seg_demean[i] < 0){
            X_seg_demean[i] = -X_seg_demean[i];
        }
    }
    double max_abs_X_seg_demean = X_seg_demean[0];
    for (int i = 1; i <size ;i++){
        if(X_seg_demean[i] > max_abs_X_seg_demean){
            max_abs_X_seg_demean = X_seg_demean[i];
        }
    }
    if(max_abs_X_seg_demean < 20){
        is_not_human = 1;
    }
    free(X_seg_demean);X_seg_demean = NULL;
    return is_not_human;
}

double obtain_resprate_with_max_possibility(const double array[],int size)
{
    double res_rate = 0.;

    res_rate = obtain_resprate_by_corr(array,size);
	  //相关法分析失败
    if(res_rate == -1)
        res_rate = obtain_resprate_by_spec(array,size);
    return res_rate;
}

double obtain_resprate_by_spec(const double array[],int size)
{
    double * pr  = (double *)malloc(size*sizeof(double));
    double * pi = (double *)malloc(size*sizeof(double));
    double * fr = (double *)malloc(size*sizeof(double));
    double * fi = (double *)malloc(size*sizeof(double));
    for (int i=0; i < size; i++)  //生成输入信号
    {
			pr[i]=array[i];
			pi[i]=0.0;
	//        fr[i]=0.0;
	//        fi[i]=0.0;
		}
    dft(pr,pi,size,fr,fi);
    //ifft(fr,fi,size,pr,pi);
    double * spectrum = (double *)malloc(size*sizeof(double));
    dft_abs(fr,fi,size,spectrum);
    //fft_abs(pr,pi,size,spectrum);
    free(pr);pr = NULL;
    free(pi);pi = NULL;
    free(fr);fr = NULL;
    free(fi);fi = NULL;
    int half_length = size/2;
    double max_value_index[2];
    max_v_spec(spectrum,max_value_index, half_length);
    free(spectrum);spectrum = NULL;
    //double max_value = max_value_index[0];
    double max_index = max_value_index[1];
    double res_rate = Fs*(max_index+1)*60/size;
    return res_rate;
}

double obtain_heart_by_spec(const double * sig,int sig_len)
{
    int Fs = 100;
    double heart_rate = -1;
    //const unsigned int S_number = 4;
    //const unsigned int num_siftings = 50;
    int f_len = sig_len - 6;
    double * sig1 = (double *)malloc(f_len*sizeof(double));
    obtain_impulse_heart_sig(sig, sig1, sig_len, f_len);
//    for(int i = 0; i < f_len; i++){
//        printf("%f\t",sig1[i]);
//    }
//    printf("\n");
//    double * sig2 = (double *)malloc(f_len*sizeof(double));
//    int M = 3;
//    double* imf = (double*)  malloc(M*f_len*sizeof(double));
//
//    int8_t use_emd = 1;
//    if(use_emd)
//        emd(sig1, f_len, imf, M, S_number, num_siftings);
//    else {
//        const size_t ensemble_size = 10;
//        const double noise_strength = 0.2;
//        const unsigned long int rng_seed = 0;
//        eemd(sig1, f_len,imf, M, ensemble_size, noise_strength, S_number, num_siftings, rng_seed);
//    }
//    free(sig1);sig1 = NULL;
//    for(int i = 0; i < f_len; i++){
//        sig2[i] = imf[i] + imf[i+f_len];
//    }
    //Data_transfer(sig2, imf, f_len, 2*f_len);
    //free(imf);imf = NULL;

    int low_hight[2];
    obtain_adaptive_filter_band(low_hight,sig1, f_len, Fs);
    double * new_sig = (double *)malloc(f_len*sizeof(double));
    if(low_hight[0] == -1){
//        low_hight[0] = 5;low_hight[1] = 15;
//        double filt_aa[19] = {1.0, -12.237650514937820745, 72.296924484907592046, -273.88411568063793311, 745.37719061919324304,
//        -1547.1603394297212617, 2537.7860098647602172, -3362.8564470711207832, 3648.859848143871659, -3265.8186162218098616,
//        2416.7094469918329196, -1475.1488672188859255, 737.28977609473031407, -297.70889608343003374, 95.011787259625663182,
//        -23.135854463679741855, 4.0503658544226999183, -0.45532469896289079481, 0.024765048464175206527};
//        double filt_bb[19] = {0.0000063519029451624906747, 0, -0.000057167126506462413531, 0, 0.00022866850602584965413, 0,
//        -0.00053355984739364924717, 0, 0.00080033977109047387075, 0, -0.00080033977109047387075, 0, 0.00053355984739364924717,
//        0, -0.00022866850602584965413, 0, 0.000057167126506462413531, 0, -0.0000063519029451624906747};
        low_hight[0] = 4;low_hight[1] = 12;
        double filt_aa[19] = {1.0, -13.668430401017703346, 89.419900736929506024, -371.98982691681464985, 1102.6016125003732213,
        -2472.5932141675334606, 4347.1788951788603299, -6126.5152203333564103, 7015.8446346289256326, -6577.0618561398114252,
        5059.4082633560492468, -3186.1417508081367487, 1630.4905252136534273, -668.91669339002578454, 215.19315881266854262,
        -52.391447514719772016, 9.0924031361717627675, -1.0040741903543359825, 0.053120374432110116503};
        double filt_bb[19] = {0.0000011176892085558873254, 0, -0.000010059202877002986141, 0, 0.000040236811508011944563, 0,
        -0.000093885893518694539571, 0, 0.00014082884027804182291, 0, -0.00014082884027804182291, 0, 0.000093885893518694539571,
        0, -0.000040236811508011944563, 0, 0.000010059202877002986141, 0, -0.0000011176892085558873254};
        filter(sig1,new_sig,f_len,filt_aa,filt_bb,19);
    }else{
        filter(sig1,new_sig,f_len,filt_a[low_hight[0]-4],filt_b[low_hight[0]-4],19);
    }
    free(sig1);sig1 = NULL;
    double * hilt_sig = (double *)malloc(f_len*sizeof(double));
    hilbert(new_sig, hilt_sig, f_len);

    free(new_sig);new_sig = NULL;
    double * indMax = (double *)malloc(200*sizeof(double));
    int * indexMax = (int *)malloc(200*sizeof(int));
    double * indMin = (double *)malloc(200*sizeof(double));
    int * indexMin = (int *)malloc(200*sizeof(int));
    int max_len=0,min_len=0;
    peakdet(hilt_sig,f_len,indMax,indexMax,indMin,indexMin,&max_len,&min_len,10.0);
	free(hilt_sig);hilt_sig = NULL;
    if(max_len <=0 || min_len <=0){
        heart_rate = -1;
        free(indMax);indMax = NULL;
        free(indexMax);indexMax = NULL;
        free(indMin);indMin = NULL;
        free(indexMin);indexMin = NULL;
        return heart_rate;
    }
    for(int i = 0; i < max_len; i++){
        indexMax[i] = indexMax[i] +1;   //加1使其与matlab结果保持一致
    }
    for(int i = 0; i < min_len; i++){
        indexMin[i] = indexMin[i] +1;  //加1使其与matlab结果保持一致
    }
    
    //有效的幅值定义
//    double MAGNITUDE_THRSH1 = 35;
//    double MAGNITUDE_THRSH2 = 0;
    double * sort_max_value = (double *)malloc(max_len*sizeof(double));
    Data_transfer(sort_max_value,indMax,0,max_len);
    sort_descend(sort_max_value,max_len);
    int top_n_0 = 0;
    if(max_len > floor(1/0.15))
        top_n_0 = floor(0.15*max_len);
    else
        top_n_0 = 1;
    int top_n_1 = floor(1*max_len);
    if(top_n_0 > top_n_1)
        top_n_0 = top_n_1;
    //int top_n_0 = floor(0.15*max_len);
    //int top_n_1 = floor(1*max_len);
    double MAGNITUDE_THRSH = 0.8*mean(sort_max_value,top_n_0-1,top_n_1);
//    if(max_len > top_n){
//        MAGNITUDE_THRSH2 = mean(sort_max_value,0,top_n);
//    }else{
//        MAGNITUDE_THRSH2 = mean(sort_max_value,0,max_len);
//    }
    free(sort_max_value);sort_max_value = NULL;
//    double MAGNITUDE_THRSH = floor(0.6*MAGNITUDE_THRSH1 + 0.4*MAGNITUDE_THRSH2);
    double DISTANCE_MIN = 50; //60/(50/100) = 120
    double DISTANCE_MAX = 200; //60/(150/100) = 30
    int start_pos = -1;
    if(max_len < 2){
        heart_rate = -1;
        free(indMax);indMax = NULL;
        free(indexMax);indexMax = NULL;
        free(indMin);indMin = NULL;
        free(indexMin);indexMin = NULL;
        return heart_rate;
    }
    start_pos = CalculateValidStartPoint(indMax,indexMax,max_len,MAGNITUDE_THRSH,DISTANCE_MIN);
    if(start_pos == -1){
        heart_rate = -1;
        free(indMax);indMax = NULL;
        free(indexMax);indexMax = NULL;
        free(indMin);indMin = NULL;
        free(indexMin);indexMin = NULL;
        return heart_rate;
    }
    int * valid_pos = (int *)malloc(max_len*sizeof(int));
    memset(valid_pos, 0x00, max_len*sizeof(int));
    int len1 = 0;
    ObtainValidPos(valid_pos,&len1,indMax,indexMax,indMin,indexMin,max_len,min_len,start_pos,
                   MAGNITUDE_THRSH,DISTANCE_MIN);
    //第二轮筛选
    double THRSH_delta = 0.9;
    if(len1 <= 2)
        THRSH_delta = 0.5;
    else if(len1 <= 4)
        THRSH_delta = 0.6;
    else if(len1 <= 6)
        THRSH_delta = 0.7;
    else if(len1 <= 8)
        THRSH_delta = 0.8;
    else
        THRSH_delta = 0.9;
    MAGNITUDE_THRSH = MAGNITUDE_THRSH * THRSH_delta;
    start_pos = CalculateValidStartPoint(indMax,indexMax,max_len,MAGNITUDE_THRSH,DISTANCE_MIN);
    int valid_pos_len = len1;
    if(start_pos != -1){
        double * indMax1 = (double *)malloc(max_len*sizeof(double));
        Data_transfer(indMax1,indMax,0,max_len);
        for(int i = 0; i < len1; i++){
            indMax1[valid_pos[i]] = 0;
        }
        int * omit_pos = (int *)malloc(max_len*sizeof(int));
        int omit_len = 0;
        ObtainValidPos(omit_pos,&omit_len,indMax1,indexMax,indMin,indexMin,max_len,min_len,start_pos,
                   MAGNITUDE_THRSH,DISTANCE_MIN);
        free(indMax1); indMax1 = NULL;
        MergeAllValidPos(valid_pos,&valid_pos_len,omit_pos,indexMax,omit_len,max_len,DISTANCE_MIN);
        free(omit_pos); omit_pos = NULL;
    }
    int * valid_max_pos = (int *)malloc(valid_pos_len*sizeof(int));
    for(int i = 0; i < valid_pos_len; i++){
        valid_max_pos[i] = indexMax[valid_pos[i]];
    }
    free(indMin);indMin = NULL;
    free(indexMin);indexMin = NULL;
    free(indMax);indMax = NULL;
    free(indexMax);indexMax = NULL;
    free(valid_pos);valid_pos = NULL;
    double * diff_max_pos = (double *)malloc((valid_pos_len-1)*sizeof(double));
    int j = 0;
    for(int i = 0; i < valid_pos_len-1; i++){
        diff_max_pos[i] = valid_max_pos[i+1] - valid_max_pos[i];
        if(diff_max_pos[i]<DISTANCE_MAX&&diff_max_pos[i]>DISTANCE_MIN){
            diff_max_pos[j++] = diff_max_pos[i];
        }
    }
    free(valid_max_pos);valid_max_pos = NULL;
    double mean_interval = -1;
    if(j > 0){
        if(j > 5){
            sort_ascend(diff_max_pos,j);
            for(int i = 1; i < j-1; i++){
                diff_max_pos[i-1] = diff_max_pos[i];
                mean_interval = mean(diff_max_pos,0,j-2);
            }
        }else{
            mean_interval = mean(diff_max_pos,0,j);
        }
    }
    if(mean_interval != -1){
        heart_rate = 60/(mean_interval/Fs);
    }else{
        heart_rate = -1;
    }
    free(diff_max_pos);diff_max_pos = NULL;
    return heart_rate;
}

double obtain_resprate_by_corr(const double array[],int size)
{
    int Nmin = 10;
    int Nmax = 50;
    //double sig_de[size];
    //Mydetrend(array, sig_de);//sig_de = detrend(array);////////////////////////////////////////////////////////没有实现
    double interval,isend;
    double para[3];
    detect_local_interval_by_peakdet(array,para,size,Nmin,Nmax);
    interval = para[0];
    //corr = para[1];
    isend = para[2];
    double temp = interval/Fs;
    double res_rate;
    if(isend == 1)
        res_rate = -1;
    else
        res_rate = 60/temp;
    return res_rate;
}

int sleep_analysis(double * heart,int len,double norm_heart,double N_thr_value){
    int cur = 1;
    double mean_heart = mean(heart,0,len);
    double std_heart = standard_deviation(heart,0,len);
    if(norm_heart - mean_heart <= 30 && norm_heart - mean_heart >= 10 && std_heart < N_thr_value){
        cur = 3;
    }
    else if(norm_heart - mean_heart < 10 && norm_heart - mean_heart >= 5){
        cur = 2;
    }
    else{
        cur = 1;
    }
    return cur;
}
