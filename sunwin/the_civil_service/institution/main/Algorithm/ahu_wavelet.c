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
#include "stdlib.h" 
#include "ahu_utils.h"
static void wextend(const double * x,double * y, int x_len,int y_len,int extend)
{
    for(int i = 0; i < extend; i++)
    {
        y[i] = x[extend - i - 1];
    }
    for(int i = extend; i < x_len + extend; i++)
    {
        y[i] = x[i - extend];
    }
    int n = 1;
    for(int i = x_len + extend; i < y_len; i++)
    {
        y[i] = x[x_len - n];
        n++;
    }
}

static void wextend_full(const double * x,double * y, int x_len,int y_len,int extend)
{
    for(int i = 0; i < extend; i++)
    {
        y[i] = 0;
    }
    for(int i = extend; i < x_len + extend; i++)
    {
        y[i] = x[i - extend];
    }
    int n = 1;
    for(int i = x_len + extend; i < y_len; i++)
    {
        y[i] = 0;
        n++;
    }
}

static void dyadup(const double * x,double * y, int y_len)
{
    //y_len = 2*x_len-1
    int n = 0;
    for(int i = 0; i < y_len; i+=2)
    {
        y[i] = x[n];
        n++;
    }
    for(int i = 1; i < y_len; i+=2)
    {
        y[i] = 0;
    }
}

static void Conv1(double * indata1, double * indata2,double * outdata, int indata1_length, int indata2_length,int outdata_length1 )
{
    //锟斤拷锟叫撅拷锟斤拷,使锟斤拷valid锟侥得碉拷锟斤拷锟斤拷锟斤拷某锟斤拷龋锟斤拷锟斤拷锟轿�1
    for (int i = 0; i < outdata_length1; i++)
    {
        outdata[i] = 0;
        for(int j = i; j < i + indata2_length; j++)
        {
            outdata[i] = outdata[i] + indata1[j]*indata2[indata2_length-1-(j-i)];
        }

    }
}

static void wconv1(const double * x, double * lo_d, double * hi_d, double * lo_out, double * hi_out, int level, int filter_len, int extend)
{
    int y_wextend_len = level+2*extend;
    double * y_wextend = (double *)malloc(sizeof(double)*y_wextend_len);
    wextend(x, y_wextend, level, y_wextend_len, extend);//锟脚猴拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷extend锟斤拷锟斤拷锟饺ｏ拷锟斤拷锟斤拷锟斤拷2*extend锟斤拷锟斤拷
    int outdata_length = y_wextend_len - filter_len + 1;//锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    double * lo_arry = (double *)malloc(sizeof(double)*outdata_length);//锟斤拷锟斤拷锟斤拷锟斤拷牡锟酵ㄏ碉拷锟斤拷锟斤拷锟斤拷诖锟�
    double * ho_arry = (double *)malloc(sizeof(double)*outdata_length);//锟斤拷锟斤拷锟斤拷锟斤拷母锟酵ㄏ碉拷锟斤拷锟斤拷锟斤拷诖锟�

    Conv1(y_wextend, lo_d, lo_arry, y_wextend_len, filter_len, outdata_length);
    int n = 0;
    for(int i = 1; i < level+filter_len-1; i+=2)
    {
        lo_out[n] = lo_arry[i];
        n++;
    }
    free(lo_arry);
    lo_arry = NULL;
    Conv1(y_wextend, hi_d, ho_arry, y_wextend_len, filter_len, outdata_length);
    int m = 0;
    for(int i = 1; i < level+filter_len-1; i+=2)
    {
        hi_out[m] = ho_arry[i];
        m++;
    }
    free(y_wextend);
    y_wextend = NULL;
    free(ho_arry);
    ho_arry = NULL;
}

static void upsconv1(double * x, double * r, double * out, int level, int filter_len, int extend)
{
    int x_dyadup_len = 2 * ((level+filter_len-1)/2) - 1;
    double * x_dyadup = (double *)malloc(sizeof(double)*x_dyadup_len);
    dyadup(x,x_dyadup,x_dyadup_len);
    int x_wextend_len = x_dyadup_len+2*extend;
    double * x_wextend = (double *)malloc(sizeof(double)*x_wextend_len);
    wextend_full(x_dyadup,x_wextend, x_dyadup_len,x_wextend_len, extend);
    free(x_dyadup);
    x_dyadup = NULL;
    int x_out_len = x_wextend_len - filter_len + 1;
    double * x_out = (double *)malloc(sizeof(double)*x_out_len);
    Conv1(x_wextend, r, x_out, x_wextend_len, filter_len, x_out_len);
    free(x_wextend);
    x_wextend = NULL;
    double longs = (x_out_len - level)/2.;
    int frist = 1 + floor(longs);
    int last = x_out_len - ceil(longs);
    int n = 0;
    for(int i = frist; i <= last; i++)
    {
        out[n] = x_out[i-1];
        n++;
    }
    free(x_out);
    x_out = NULL;
}

//小锟斤拷锟戒换3锟斤拷锟街斤拷wavedec
void wavedec(const double * ori_sign, double * out_sign, int * out_levels,int sign_len,int levels_len)
{
    double lo_d[] = {-0.00338241595100613,-0.000542132331791148,0.0316950878114930,0.00760748732491761,-0.143294238350810,-0.0612733590676585,
    0.481359651258372,0.777185751700524,0.364441894835331,-0.0519458381077090,-0.0272190299170560,0.0491371796736075,0.00380875201389062,
    -0.0149522583370482,-0.000302920514721367,0.00188995033275946};//锟街斤拷牡锟酵拷瞬锟斤拷锟较碉拷锟�
    double hi_d[] = {-0.00188995033275946,-0.000302920514721367,0.0149522583370482,0.00380875201389062,-0.0491371796736075,-0.0272190299170560,
    0.0519458381077090,0.364441894835331,-0.777185751700524,0.481359651258372,0.0612733590676585,-0.143294238350810,-0.00760748732491761,
    0.0316950878114930,0.000542132331791148,-0.00338241595100613};//锟街斤拷母锟酵拷瞬锟斤拷锟较碉拷锟�
    int filter_len = 16;//锟剿诧拷锟斤拷锟斤拷锟斤拷
    int extend = filter_len-1;
    out_levels[levels_len-1]= sign_len;

    //一锟斤拷锟街斤拷
    double * lo_out1 = (double *)malloc(sizeof(double)*(out_levels[levels_len-1]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    double * hi_out1 = (double *)malloc(sizeof(double)*(out_levels[levels_len-1]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    out_levels[levels_len-2]= (sign_len+filter_len-1)/2;
    wconv1(ori_sign, lo_d, hi_d, lo_out1, hi_out1, out_levels[levels_len-1], filter_len, extend);

    //锟斤拷锟斤拷锟街斤拷
    double * lo_out2 = (double *)malloc(sizeof(double)*(out_levels[levels_len-2]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    double * hi_out2 = (double *)malloc(sizeof(double)*(out_levels[levels_len-2]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    out_levels[levels_len-3]= (out_levels[levels_len-2]+filter_len-1)/2;
    wconv1(lo_out1, lo_d, hi_d, lo_out2, hi_out2, out_levels[levels_len-2], filter_len, extend);

    //锟斤拷锟斤拷锟街斤拷
    double * lo_out3 = (double *)malloc(sizeof(double)*(out_levels[levels_len-3]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    double * hi_out3 = (double *)malloc(sizeof(double)*(out_levels[levels_len-3]+filter_len-1)/2);//1锟斤拷锟斤拷通锟剿诧拷系锟斤拷
    out_levels[levels_len-4]= (out_levels[levels_len-3]+filter_len-1)/2;
    wconv1(lo_out2, lo_d, hi_d, lo_out3, hi_out3, out_levels[levels_len-3], filter_len, extend);
    out_levels[levels_len-5]= (out_levels[levels_len-3]+filter_len-1)/2;


    //锟街斤拷之锟斤拷姆纸锟较碉拷锟轿猍lo_arry3;ho_arry3;ho_arry2;ho_arry1]
    for(int i = 0; i < (out_levels[0]+out_levels[1]+out_levels[2]+out_levels[3]); i++)
    {
        if(i < out_levels[0]){
            out_sign[i] = lo_out3[i];
        }
        if((i < (out_levels[0]+out_levels[1]))&&(i >= out_levels[0])){
            out_sign[i] = hi_out3[i-out_levels[0]];
        }
        if((i < (out_levels[0]+out_levels[1]+out_levels[2]))&&(i >= out_levels[0]+out_levels[1])){
            out_sign[i] = hi_out2[i-out_levels[0]-out_levels[1]];
        }
        if(i >= (out_levels[0]+out_levels[1]+out_levels[2])){
            out_sign[i] = hi_out1[i-out_levels[0]-out_levels[1]-out_levels[2]];
        }
    }
    free(lo_out1);
    lo_out1 = NULL;
    free(lo_out2);
    lo_out2 = NULL;
    free(lo_out3);
    lo_out3 = NULL;
    free(hi_out1);
    hi_out1 = NULL;
    free(hi_out2);
    hi_out2 = NULL;
    free(hi_out3);
    hi_out3 = NULL;
}

//小锟斤拷锟戒换锟截癸拷waverec
void waverec(const double * dec_details,int * levels,double * rec_sign)
{
    double lo_r[] = {0.00188995033275946,-0.000302920514721367,-0.0149522583370482,0.00380875201389062,0.0491371796736075,-0.0272190299170560,
    -0.0519458381077090,0.364441894835331,0.777185751700524,0.481359651258372,-0.0612733590676585,-0.143294238350810,0.00760748732491761,
    0.0316950878114930,-0.000542132331791148,-0.00338241595100613};//锟截癸拷锟侥碉拷通锟剿诧拷锟斤拷系锟斤拷
    double hi_r[] = {-0.00338241595100613,0.000542132331791148,0.0316950878114930,-0.00760748732491761,-0.143294238350810,0.0612733590676585,
    0.481359651258372,-0.777185751700524,0.364441894835331,0.0519458381077090,-0.0272190299170560,-0.0491371796736075,0.00380875201389062,
    0.0149522583370482,-0.000302920514721367,-0.00188995033275946};//锟截癸拷锟侥革拷通锟剿诧拷锟斤拷系锟斤拷
    int filter_len = 16;//锟剿诧拷锟斤拷锟斤拷锟斤拷
    int extend = filter_len-1;
    /**锟斤拷锟斤拷锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    double * a3 = (double *)malloc(sizeof(double)*levels[0]);//extract detail
    for(int i = 0; i < levels[0]; i++){
        a3[i] = dec_details[i];
    }
    double * lo_out3 = (double *)malloc(sizeof(double)*levels[2]);
    upsconv1(a3, lo_r, lo_out3, levels[2], filter_len, extend);
    free(a3);
    a3 = NULL;
    //锟斤拷通锟剿诧拷锟截癸拷
    double * d3 = (double *)malloc(sizeof(double)*levels[1]);//extract detail
    for(int i = 0; i < levels[1]; i++){
        d3[i] = dec_details[i+levels[0]];
    }
    double * hi_out3 = (double *)malloc(sizeof(double)*levels[2]);
    upsconv1(d3, hi_r, hi_out3, levels[2], filter_len, extend);
    free(d3);
    d3 = NULL;
    double * rec_sign3 = (double *)malloc(sizeof(double)*levels[2]);
    for(int i = 0; i < levels[2]; i++)
    {
        rec_sign3[i] = lo_out3[i] + hi_out3[i];
    }
    free(lo_out3);
    lo_out3 = NULL;
    free(hi_out3);
    hi_out3 = NULL;
    /**锟节讹拷锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    double * lo_out2 = (double *)malloc(sizeof(double)*levels[3]);
    upsconv1(rec_sign3, lo_r, lo_out2, levels[3], filter_len, extend);
    free(rec_sign3);
    rec_sign3 = NULL;
    //锟斤拷通锟剿诧拷锟截癸拷
    double * d2 = (double *)malloc(sizeof(double)*levels[2]);//extract detail
    for(int i = 0; i < levels[2]; i++){
        d2[i] = dec_details[i+levels[0]+levels[1]];
    }
    double * hi_out2 = (double *)malloc(sizeof(double)*levels[3]);
    upsconv1(d2, hi_r, hi_out2, levels[3], filter_len, extend);
    free(d2);
    d2 = NULL;
    double * rec_sign2 = (double *)malloc(sizeof(double)*levels[3]);
    for(int i = 0; i < levels[3]; i++)
    {
        rec_sign2[i] = lo_out2[i] + hi_out2[i];
    }
    free(lo_out2);
    lo_out2 = NULL;
    free(hi_out2);
    hi_out2 = NULL;
    /**锟斤拷一锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    double * lo_out1 = (double *)malloc(sizeof(double)*levels[4]);
    upsconv1(rec_sign2, lo_r, lo_out1, levels[4], filter_len, extend);
    free(rec_sign2);
    rec_sign2 = NULL;
    //锟斤拷通锟剿诧拷锟截癸拷
    double * d1 = (double *)malloc(sizeof(double)*levels[3]);//extract detail
    for(int i = 0; i < levels[3]; i++){
        d1[i] = dec_details[i+levels[0]+levels[1]+levels[2]];
    }
    double * hi_out1 = (double *)malloc(sizeof(double)*levels[4]);
    upsconv1(d1, hi_r, hi_out1, levels[4], filter_len, extend);
    free(d1);
    d1 = NULL;
    for(int i = 0; i < levels[4]; i++)
    {
        rec_sign[i] = lo_out1[i] + hi_out1[i];
    }
    free(lo_out1);
    lo_out1 = NULL;
    free(hi_out1);
    hi_out1 = NULL;
}

//小锟斤拷锟戒换锟截癸拷wrcoef
void wrcoef(const double * dec_details,int * levels,double * rec_sign)
{
    double lo_r[] = {0.00188995033275946,-0.000302920514721367,-0.0149522583370482,0.00380875201389062,0.0491371796736075,-0.0272190299170560,
    -0.0519458381077090,0.364441894835331,0.777185751700524,0.481359651258372,-0.0612733590676585,-0.143294238350810,0.00760748732491761,
    0.0316950878114930,-0.000542132331791148,-0.00338241595100613};//锟截癸拷锟侥碉拷通锟剿诧拷锟斤拷系锟斤拷
    int filter_len = 16;//锟剿诧拷锟斤拷锟斤拷锟斤拷
    int extend = filter_len-1;
    /**锟斤拷锟斤拷锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    double * a3 = (double *)malloc(sizeof(double)*levels[0]);//extract detail
    for(int i = 0; i < levels[0]; i++){
        a3[i] = dec_details[i];
    }
    double * lo_out3 = (double *)malloc(sizeof(double)*levels[2]);
    upsconv1(a3, lo_r, lo_out3, levels[2], filter_len, extend);
    free(a3);
    a3 = NULL;
    /**锟节讹拷锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    double * lo_out2 = (double *)malloc(sizeof(double)*levels[3]);
    upsconv1(lo_out3, lo_r, lo_out2, levels[3], filter_len, extend);
    free(lo_out3);
    lo_out3 = NULL;
     /**锟斤拷一锟斤拷锟截癸拷*/
    //锟斤拷通锟剿诧拷锟截癸拷
    upsconv1(lo_out2, lo_r, rec_sign, levels[4], filter_len, extend);
    free(lo_out2);
    lo_out2 = NULL;
}

