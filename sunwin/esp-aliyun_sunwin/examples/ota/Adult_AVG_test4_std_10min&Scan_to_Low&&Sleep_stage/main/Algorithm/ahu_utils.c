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
void sort_descend(double array[],int size) //闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟閭﹀枟閺嗘粓鎮楀☉娅虫垹绱為崶顒佺厽闁逛即娼ф晶顔姐亜韫囨挾鍩ｉ柟顔角圭粻娑㈠箻閸撲緡鍟嬮梻渚€鈧偛鑻晶顔姐亜閿旂偓鏆€殿噮鍋婇獮妯兼嫚閸欏妫熼梻渚€娼ч悧鍡椢涘Δ鍜佹晜闁割偅娲橀埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷绀侀幖顐﹀磹閸洖纾归柡宥庡亐閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈呭窛濠电姴瀚槐鏇㈡⒒娴ｅ摜绉烘い銉︽崌瀹曟顫滈埀顒€顕ｉ锕€绠婚悹鍥у级椤ユ繈姊洪棃娑氬婵☆偅顨婇、鏃堟晸閿燂拷
{
    double temp;//闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨跺娲垂椤曞懎鍓卞銇礁娲﹂埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷绀侀幖顐﹀磹閸洖纾归柡宥庡亐閸嬫挸顫濋悙顒€顏�
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
void sort_ascend(double array[],int size) //闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨抽埀顒傛嚀鐎氼厽绔熼崱妞ユ帡宕奸妷锔规嫼闁荤姵浜介崝宀勫几閹达附鐓涢悗锝庝憾閻撳吋顨ラ悙鑼闁诡喗绮撻幊鐐哄Ψ閿旂瓔浠ч梻鍌欑閹碱偊宕愰崼鏇炵９闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮嫰閹冲繐顕ラ崟顖氱疀妞ゆ挾鍠愰鐔兼⒒娴ｅ憡鎯堥柛濠傜埣瀹曟劙寮介鐔蜂壕婵﹩鍓涚粔娲煛鐏炶濮傞柟顔哄灲瀹曘劍绻濋崟顏嗙？闂傚倷绀佺紞濠囁夐幘璇茬婵せ鍋撶€殿噮鍋婇獮妯兼嫚閸欏妫熼梻渚€娼ч悧鍡椢涘Δ鍜佹晜闁割偅娲橀埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷绀侀幖顐﹀磹閸洖纾归柡宥庡亐閸嬫挸顫濋悙顒€顏�
{
    double temp;//闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨跺娲垂椤曞懎鍓卞銇礁娲﹂埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷绀侀幖顐﹀磹閸洖纾归柡宥庡亐閸嬫挸顫濋悙顒€顏�
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
    int temp;//闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨跺娲垂椤曞懎鍓卞銇礁娲﹂埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷绀侀幖顐﹀磹閸洖纾归柡宥庡亐閸嬫挸顫濋悙顒€顏�
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
    double avg = mean(array,len1,len2);//濠电姵顔栭崰姘跺箠閹捐纾归柟闂寸劍閻撱垽鏌嶉崫鍕櫣闂佽￥鍊濋弻娑㈠Ψ閹存柨浜鹃梺绋款儐閹瑰洭寮幘缁樻櫢闁跨噦鎷�
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
//matlab闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨崇槐鎺斾沪缁涘鎮猣f闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨堕弻锝夊箣閿濆憛鎾绘煛閸涱喗鍊愰柡宀嬬節瀹曟帒螣鐞涒€充壕闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈呭窛濠电姴瀚槐鏇㈡⒒娴ｇ儤鍤€闁搞倖鐗犻獮蹇涙晸閿燂拷
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
    ret_buf[j++] = fabs(dat[i]-dat[i-1]);//婵犳鍠氶幊鎾诲磹閸︻厽瀚氶柣鎰劋閸婄兘鎮峰▎蹇擃仾婵炲牆澧庣槐鎾存媴閸︻厾鐣靛銈嗘处閸ｏ絽鐣烽幇鐗堟櫢闁跨噦鎷�
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

/** 闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨堕弻锝夊箣閿濆憛鎾绘煛閸涱喗鍊愰柡宀嬬節瀹曟帒螣鐞涒€充壕闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈嗗€绘俊顖濇椤斿檧akdet闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨堕弻锝夊箣閿濆憛鎾绘煛閸涱喗鍊愰柡宀嬬節瀹曟帒螣鐞涒€充壕闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈呭窛濠电姴瀚槐鏇㈡⒒娴ｇ儤鍤€闁搞垺鐓″畷鏌ュ蓟閵夈儳鍔﹀銈嗗坊閸嬫捇鏌曢崼鐔稿€愮€殿噮鍋婇獮妯兼嫚閸欏妫熼梻渚€娼ч悧鍡椢涘Δ鍜佹晜闁肩ǹ鐏氶崣蹇涙煥濠靛棙鍣界紒鐘差煼閺屸剝鎷呯憴鍕３閻庢鍣崜鐔镐繆閸洖骞㈡俊銈咃梗缁憋箓姊婚崒娆愮グ婵炲娲熷畷浼村箛閺夊灝鍤戞繝鐢靛У閼瑰墽绮婚婧惧亾閻熸澘顏鐟邦儔瀹曟垿骞樼€垫悂妾梺鍛婄☉閿曘儱鈻嶉姀銈嗏拺闁告繂瀚埀顒€鐖煎畷鎰板冀椤愮喎浜炬慨妯诲墯濞兼劙鏌熼娑欘棃鐎殿噮鍣ｅ畷濂告偄閸涘⿴浠ч梻鍌欑閹碱偊宕愰崼鏇炵９闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈呭窛濠电姴瀚槐鏇㈡⒒娴ｅ摜绉烘い銉︽崌瀹曟顫滈埀顒€顕ｉ锕€绠婚悹鍥у级椤ユ繈姊洪棃娑氬婵☆偅顨婇、鏃堝醇閺囩啿鎷洪柣鐘充航閸斿矂寮稿▎鎰╀簻妞ゆ劧缍嗗▓姗€鏌℃笟鍥ф珝妞ゃ垺鐟╅幐濠冨緞濡湱澶刢闂傚倸鍊峰ù鍥р枖閺囥垹绐楅柟鐗堟緲閸戠姴鈹戦悩瀹犲缂佺媭鍨堕弻锝夊箣閿濆憛鎾绘煛閸涱喗鍊愰柡宀嬬節瀹曟帒螣鐞涒€充壕闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈嗗殥闁靛牆娲ㄥ畷娲⒑濮瑰洤鐒洪柛銊ャ偢瀹曟澘螖閸愶絽浜炬慨姗嗗墰缁夌儤顨ラ悙鎼當閻撱倖銇勮箛鎾村櫤婵炲懌鍨藉娲传閸曨偀鍋撻崼鏇炵９闁哄稁鍋€閸嬫挸顫濋鍌溞ㄩ梺鍝勮閸旀垿骞冮姀銈呭窛濠电姴瀚槐鏇㈡⒒娴ｅ摜绉烘い銉︽崌瀹曟顫滈埀顒€顕ｉ锕€绠婚悹鍥у级椤ユ繈姊洪棃娑氬婵☆偅顨婇、鏃堝醇閺囩喎浠梺鍝勬处濮樸劎绮ｉ弮鍫熺厸濞达綀顫夊畷宀勬煙椤栨氨鐒告い銏℃礋婵¤埖鎯旈姀鈽嗗敹闂傚倷绀佺紞濠囁夐幘璇茬婵せ鍋撶€殿噮鍋婇獮妯兼嫚閸欏妫熼梻渚€娼ч悧鍡椢涘Δ鍜佹晜闁割偅娲橀悡鏇炩攽閻樻彃鈧爼宕戦姀銈嗙厸濞达綀顫夊畷宀€鈧鍣崜鐔镐繆閸洖骞㈡俊銈咃梗缁憋箓姊婚崒娆愮グ婵炲娲熷畷浼村箛閺夊灝鍤戞繝鐢靛У閼瑰墽绮婚鐐寸厽闁硅揪绲借闂佸搫鎳忛幃鍌炲蓟閿熺姴纾兼俊顖濄€€閸嬫捇寮介鐔蜂壕婵﹩鍓涚粔娲煛鐏炶濮傞柟顔哄灮缁瑧鎹勯妸锔锯偓顓㈡⒑濮瑰洤鐒洪柛銊ャ偢瀹曟澘螖閸愶絽浜炬慨姗€纭搁崝姝泃lab婵犵數鍋為崹鍫曞箰閹绢喖纾婚柟鍓х帛閳锋垿鎮归幁鎺戝闁哄鐩弻鐔虹驳鐎ｂ晠鍋楅梺杞扮劍閸旀绮诲☉銏犳婵炲棗褰炵槐锕傛⒒閸屾瑦绁版繛澶嬫礋瀹曚即骞囬弶鍨殤婵犵數濮甸懝鍓х不椤栫偞鐓熼柟杈剧到琚氶梺鍝勬噺閹倿寮婚敐澶樻晣鐟滃秵绂嶉悙娣簻闁挎繂楠稿▍宥嗩殽閻愯尙绠婚柡灞诲妿閳ь剨缍嗛崑鎺懳ｉ鐐粹拻濞达綀濮ょ涵鍫曟煕閻樺啿濮嶇€殿喖鎲＄换婵嗩潩椤掑偆鍞甸梻鍌欑閻忔繈顢栭崱娑樻辈闁割偁鍎查埛鎴︽偣閹帒濡奸柡瀣灴閺岋紕鈧綆浜堕悡鍏碱殽閻愯尙绠婚柟顔界矒閹崇偤濡烽敂绛嬩户闂傚倷娴囧▔鏇㈠闯閿曞倸绠柨鐕傛嫹1*/
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
