#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_system.h"

#define DAC_EXAMPLE_CHANNEL     CONFIG_EXAMPLE_DAC_CHANNEL
#define ADC2_EXAMPLE_CHANNEL    CONFIG_EXAMPLE_ADC2_CHANNEL//模拟转数字

void init_0(double array[] ,int size);
void Data_transfer(double array1[],const double array2[],int size1,int size2);
double onearray_sum(double array[],int size);
double accumulate1(const double array[], int size,int seg);
double accumulate2(const double array1[], const double array2[],int size);
double obtain_resprate_by_spec(const double array[],int size,int seg);
double obtain_resprate_by_corr(const double array[],int size);
void detect_local_interval(const double array[],double a[],int size,int Nmin,int Nmax);
void max_v_interval(const double array[],double maxValueIdex[],int size);
void max_v_spec(const double array[],double maxValueIdex[],int size);
double median(const double array[],int size);
void fft(double pr[],double pi[],int n,int k,double fr[],double fi[]);
void wextend(const double * x,double * y, int x_len,int y_len,int extend);//信号左右沿周期扩充
void wkeep1(const double * x,double * y, int x_len,int y_len);//抽取信号中间值
void wextend_full(const double * x,double * y, int x_len,int y_len,int extend);//左右填充0；
void dyadup(const double * x,double * y, int y_len);//中间偶数为补0；
void Conv1(double * indata1, double * indata2,double * outdata, int indata1_length, int indata2_length,int outdata_length1);//卷积
void wavedec_wrcoef(const double * ori_sign, double * out_sign, int sign_len);//分解小波和重构

void app_main(void)
{
    uint64_t tt = 0;
    int output_data=0;
    uint8_t time=0;
    uint32_t num1=0,num2=0,num3=0;
    int read_raw1, read_raw2, read_raw3;
    uint64_t data_index = 0;
    unsigned char begin1 = 0;
    unsigned char begin2 = 0;
    unsigned char N_seg = 100;//每段信号100个数据点
    //int N_overlay = 30;//重采样
    unsigned char adative_range = 12;//最开始的2分钟 12*10 = 2 分钟
    double adaptive_energy_thresh = -1;//用来检测是否有体动
    double adaptive_resp_normal = -1; //用来检测正常的呼吸频率
    //int Total_seg = (row-N_overlay)/(N_seg-N_overlay);//总的段数
    double seg_energy_vec;
    double rate_value_vec = 0.;
    double rate_value_vec1;
    double rate_value_vec2;
    int adative_range_index = 0;
    double * median_energy_vec = (double *)malloc(adative_range*sizeof(double));
    init_0(median_energy_vec,adative_range);
    double * median_resp_vec = (double *)malloc(adative_range*sizeof(double));
    init_0(median_resp_vec,adative_range);
    double * X_X = (double *)malloc(100*sizeof(double));   //每次取100个数据点进行检测
    double event_type_vec;// 0 正常 1 低通气 2 睡眠暂停 3 呼吸过快 4 坠床 5 体动
    esp_err_t r,r1,r2,r3;

    gpio_num_t adc_gpio_num1,adc_gpio_num2, adc_gpio_num3;

    r = adc2_pad_get_io_num( ADC2_CHANNEL_5, &adc_gpio_num1 );
    assert( r == ESP_OK );
     r = adc2_pad_get_io_num( ADC2_CHANNEL_6, &adc_gpio_num2 );
    assert( r == ESP_OK );
     r = adc2_pad_get_io_num( ADC2_CHANNEL_7, &adc_gpio_num3 );
    assert( r == ESP_OK );

    //be sure to do the init before using adc2.

    adc2_config_channel_atten( ADC2_CHANNEL_5, ADC_ATTEN_11db );
    adc2_config_channel_atten( ADC2_CHANNEL_6, ADC_ATTEN_11db );
    adc2_config_channel_atten( ADC2_CHANNEL_7, ADC_ATTEN_11db );
    vTaskDelay(2 * portTICK_PERIOD_MS);


    //printf("start conversion.\n");
    printf("Sleep after two minutes:");
    while(1) {
        event_type_vec = 0;
        vTaskDelay(18/10/ portTICK_PERIOD_MS);
       time++;
        tt++;
        r1 = adc2_get_raw( ADC2_CHANNEL_5, ADC_WIDTH_12Bit, &read_raw1);//压电
        r2 = adc2_get_raw( ADC2_CHANNEL_6, ADC_WIDTH_12Bit, &read_raw2);//压阻
        r3 = adc2_get_raw( ADC2_CHANNEL_7, ADC_WIDTH_12Bit, &read_raw3);//VREF基准
       num1=num1+read_raw1;
       num2=num2+read_raw2;
       num3=num3+read_raw3;
        if ((r1 == ESP_OK)&&(r2 == ESP_OK)&&(r3 == ESP_OK)&&(time==10) ){
            output_data++;
            time=0;
            read_raw1=num1/10;
            read_raw2=num2/10;
            read_raw3=num3/10;
            num1=0;num2=0;num3=0;
            printf("%d:      PV:  %d      PR:  %d      VREF:  %d  \n", output_data, read_raw1,read_raw2, read_raw3 );
            data_index++;
        }else if ( r1== ESP_ERR_INVALID_STATE ) {
            //printf("%s: ADC2 not initialized yet.\n", esp_err_to_name(r1));
        } else if ( r1 == ESP_ERR_TIMEOUT ) {
            //This can not happen in this example. But if WiFi is in use, such error code could be returned.
         //   printf("%s: ADC2 is in use by Wi-Fi.\n", esp_err_to_name(r1));
        } else {
          //  printf("%s\n", esp_err_to_name(r1));
        }

        //开始的信号不用，用于获取一些自适应的参数，取前两分钟
        if((data_index <= 1200)&&(tt %10 == 0)){
            X_X[begin1] = read_raw1 - read_raw3;
            begin1++;
            if(begin1==N_seg){
                begin1 = 0;
                double * a1 = (double *)malloc(N_seg*sizeof(double));
                wavedec_wrcoef(X_X,a1,N_seg);
                median_energy_vec[adative_range_index] = accumulate1(a1,N_seg,N_seg);
                rate_value_vec2 = obtain_resprate_by_corr(a1,N_seg);
                if (rate_value_vec2 == -1){ // 相关法分析失败
                    rate_value_vec1 = obtain_resprate_by_spec(a1,N_seg,N_seg);
                    median_resp_vec[adative_range_index] = rate_value_vec1;
                }else{
                    median_resp_vec[adative_range_index] = rate_value_vec2;
                }
                adative_range_index++;
                free(a1);
            }
            if(data_index==1200){
                adaptive_energy_thresh = median(median_energy_vec,adative_range);
                adaptive_resp_normal = median(median_resp_vec,adative_range);
            }
        }


        if((data_index > 1200)&&(tt % 10 == 0)){
            X_X[begin2] = read_raw1 - read_raw3;
            begin2++;
            if(begin2==N_seg){
                clock_t start, finish;//计算算法的时间
                double  duration;
                start = clock();
                begin2 = 0;
                double * a2 = (double *)malloc(N_seg*sizeof(double));
                wavedec_wrcoef(X_X,a2,N_seg);//小波变换
                seg_energy_vec = accumulate1(a2,N_seg,N_seg);
                if (seg_energy_vec > 2*adaptive_energy_thresh){
                    event_type_vec = 5; // 5 体动
                }
                if (seg_energy_vec < (1.0/5)*adaptive_energy_thresh){
                    event_type_vec = 2;  // 2 睡眠暂停
                }

                //计算呼吸频率
                rate_value_vec2 = obtain_resprate_by_corr(a2,N_seg);
                if (rate_value_vec2 == -1){      // 相关法分析失败
                    if (event_type_vec == 0){ //如果呼吸信号正常
                        rate_value_vec1 = obtain_resprate_by_spec(a2,N_seg,N_seg);
                        rate_value_vec = rate_value_vec1;
                    }
                }
                else{
                    rate_value_vec = rate_value_vec2;
                }
                //判断当前的类型
                if (rate_value_vec > 1.5*adaptive_resp_normal && event_type_vec == 0){
                    event_type_vec = 3; //3 呼吸过快
                }
                if (rate_value_vec < 1.5*adaptive_resp_normal && rate_value_vec > 0.5*adaptive_resp_normal){
                    if (seg_energy_vec < (1.0/2)*adaptive_energy_thresh){
                        event_type_vec = 1; //1 低通气
                    }
                }
                free(a2);
                printf("\n%llusecond~",(data_index - 1200)/10-9);
                printf("%llusecond: ",(data_index - 1200)/10);
                printf("rate_value_vec：%f",rate_value_vec);
                // if(event_type_vec == 0)
                //     printf("normal\n");
                // if(event_type_vec == 1)
                //     printf("Low ventilation\n");
                // if(event_type_vec == 2)
                //     printf("Sleep to suspend\n");
                // if(event_type_vec == 3)
                //     printf("Breathing too fast\n");
                // if(event_type_vec == 4)
                //     printf("Drop of bed\n");
                // if(event_type_vec == 5)
                //     printf("Body moving\n");
                finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC;
                printf( "\n算法时间：%f seconds\n", duration );
            }
        }
    }
    free(median_energy_vec);
    free(median_resp_vec);
    free(X_X);
}

void init_0(double array[] ,int size)
{
    for(int i = 0; i < size; i++){
        array[i] = 0;
    }
}

void Data_transfer(double array1[],const double array2[],int size1,int size2)
{
    for(int i = size1; i < size2; i++){
        array1[i-size1] = array2[i];
    }
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
    free(temp);
}
double onearray_sum(double array[],int size)
{
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum = sum + array[i];
    }
    return sum;
}
double median(const double array[],int size)
{
    double  * array1 = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++)
        {
            array1[i] = array[i];
        }
    double temp;
    double mid;
    for(int i = 0; i < size-1; i++)
    {
        for(int j = 0 ; j < size; j++)
        {
            if(array1[j] < array1[j+1]){
                temp = array1[j];
                array1[j] = array1[j+1];
                array1[j+1] = temp;
            }
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
    free(mul);
    free(mul_divide);
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
    free(mul1);
    free(mul2);
    free(mul3);
    return result;
}
void detect_local_interval(const double array[],double a[],int size,int Nmin,int Nmax)
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
        free(seg_left);
        free(seg_right);
    }
    double max_value_index[2];
    max_v_interval(corr_vec,max_value_index, temp);
    free(corr_vec);
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
double obtain_resprate_by_spec(const double array[],int size,int seg)
{
    int Fs = 10;
    double * pr  = (double *)malloc(size*sizeof(double));
    double * pi = (double *)malloc(size*sizeof(double));
    double * fr = (double *)malloc(size*sizeof(double));
    double * fi = (double *)malloc(size*sizeof(double));
    for (int i=0; i < size; i++)  //生成输入信号
    {
		pr[i]=array[i];
		pi[i]=0.0;
        fr[i]=0.0;
        fi[i]=0.0;
	}
    fft(pr,pi,size,6,fr,fi);
    int half_length = size/2;
    double max_value_index[2];
    max_v_spec(pr,max_value_index, half_length);
    //double max_value = max_value_index[0];
    int max_index = max_value_index[1];
    double res_rate = Fs*(max_index+1)*60/seg;
    free(pr);
    free(pi);
    free(fr);
    free(fi);
    return res_rate;
}
double obtain_resprate_by_corr(const double array[],int size)
{
    int Fs = 10;
    int Nmin = 10;
    int Nmax = 50;
    //double sig_de[size];
    //Mydetrend(array, sig_de);//sig_de = detrend(array);////////////////////////////////////////////////////////没有实现
    double interval,isend;
    double para[3];
    detect_local_interval(array,para,size,Nmin,Nmax);
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
void fft(double pr[],double pi[],int n,int k,double fr[],double fi[])
{
    // if(n == pow(2,k))
    // {
    //     int it,m,is,i,j,nv,l0;
    //     double p,q,s,vr,vi,poddr,poddi;
    //     for (it=0; it<=n-1; it++)  //将pr[0]和pi[0]循环赋值给fr[]和fi[]
    //     {
    //         m=it;
    //         is=0;
    //         for(i=0; i<=k-1; i++)
    //         {
    //             j=m/2;
    //             is=2*is+(m-2*j);
    //             m=j;
    //         }
    //         fr[it]=pr[is];
    //         fi[it]=pi[is];
    //     }
    //     pr[0]=1.0;
    //     pi[0]=0.0;
    //     p=6.283185306/(1.0*n);
    //     pr[1]=cos(p); //将w=e^-j2pi/n用欧拉公式表示
    //     pi[1]=-sin(p);

    //     for (i=2; i<=n-1; i++)  //计算pr[]
    //     {
    //         p=pr[i-1]*pr[1];
    //         q=pi[i-1]*pi[1];
    //         s=(pr[i-1]+pi[i-1])*(pr[1]+pi[1]);
    //         pr[i]=p-q; pi[i]=s-p-q;
    //     }
    //     for (it=0; it<=n-2; it=it+2)
    //     {
    //         vr=fr[it];
    //         vi=fi[it];
    //         fr[it]=vr+fr[it+1];
    //         fi[it]=vi+fi[it+1];
    //         fr[it+1]=vr-fr[it+1];
    //         fi[it+1]=vi-fi[it+1];
    //     }
    //     m=n/2;
    //     nv=2;
    //     for (l0=k-2; l0>=0; l0--) //蝴蝶操作
    //     {
    //         m=m/2;
    //         nv=2*nv;
    //         for (it=0; it<=(m-1)*nv; it=it+nv)
    //           for (j=0; j<=(nv/2)-1; j++)
    //             {
    //                 p=pr[m*j]*fr[it+j+nv/2];
    //                 q=pi[m*j]*fi[it+j+nv/2];
    //                 s=pr[m*j]+pi[m*j];
    //                 s=s*(fr[it+j+nv/2]+fi[it+j+nv/2]);
    //                 poddr=p-q;
    //                 poddi=s-p-q;
    //                 fr[it+j+nv/2]=fr[it+j]-poddr;
    //                 fi[it+j+nv/2]=fi[it+j]-poddi;
    //                 fr[it+j]=fr[it+j]+poddr;
    //                 fi[it+j]=fi[it+j]+poddi;
    //             }
    //     }
    //     for (i=0; i<=n-1; i++)
    //        {
    //           pr[i]=sqrt(fr[i]*fr[i]+fi[i]*fi[i]);  //幅值计算
    //        }
    // }
    //else{
    double p = 6.283185306/(1.0*n);
    for(int i= 1; i <= n; i++)
    {
        for(int j= 1; j <= n; j++)
        {
            fr[i-1] = fr[i-1] + pr[j-1]*cos(p*(i-1)*(j-1)) + pi[j-1]*sin(p*(i-1)*(j-1));
            fi[i-1] = fi[i-1] + pr[j-1]*(-sin(p*(i-1)*(j-1))) + pi[j-1]*cos(p*(i-1)*(j-1));
        }
    }
    for (int i=0; i<=n-1; i++)
    {
        pr[i]=sqrt(fr[i]*fr[i]+fi[i]*fi[i]);  //幅值计算
    }
}
void wextend(const double * x,double * y, int x_len,int y_len,int extend)
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

void wkeep1(const double * x,double * y, int x_len,int y_len)
{
    double d = (x_len-y_len)/2;
    int frist = 1 + floor(d);
    int last = x_len - ceil(d);
    int n = 0;
    for(int i = frist; i <= last; i++)
    {
        y[n] = x[i];
        n++;
    }
}

void wextend_full(const double * x,double * y, int x_len,int y_len,int extend)
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

void dyadup(const double * x,double * y, int y_len)
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

void Conv1(double * indata1, double * indata2,double * outdata, int indata1_length, int indata2_length,int outdata_length1 )
{
    //进行卷积,使用valid的得到卷积后的长度，步长为1
    for (int i = 0; i < outdata_length1; i++)
    {
        outdata[i] = 0;
        for(int j = i; j < i + indata2_length; j++)
        {
            outdata[i] = outdata[i] + indata1[j]*indata2[indata2_length-1-(j-i)];
        }

    }
}
void wavedec_wrcoef(const double * ori_sign, double * out_sign, int sign_len)
{
    double lo_d[] = {-0.00338241595100613,-0.000542132331791148,0.0316950878114930,0.00760748732491761,-0.143294238350810,-0.0612733590676585,
    0.481359651258372,0.777185751700524,0.364441894835331,-0.0519458381077090,-0.0272190299170560,0.0491371796736075,0.00380875201389062,
    -0.0149522583370482,-0.000302920514721367,0.00188995033275946};//分解的低通滤波器系数
    double hi_d[] = {-0.00188995033275946,-0.000302920514721367,0.0149522583370482,0.00380875201389062,-0.0491371796736075,-0.0272190299170560,
    0.0519458381077090,0.364441894835331,-0.777185751700524,0.481359651258372,0.0612733590676585,-0.143294238350810,-0.00760748732491761,
    0.0316950878114930,0.000542132331791148,-0.00338241595100613};//分解的高通滤波器系数
    double lo_r[] = {0.00188995033275946,-0.000302920514721367,-0.0149522583370482,0.00380875201389062,0.0491371796736075,-0.0272190299170560,
    -0.0519458381077090,0.364441894835331,0.777185751700524,0.481359651258372,-0.0612733590676585,-0.143294238350810,0.00760748732491761,
    0.0316950878114930,-0.000542132331791148,-0.00338241595100613};//重构的低通滤波器系数
    //double hi_r[] = {-0.00338241595100613,0.000542132331791148,0.0316950878114930,-0.00760748732491761,-0.143294238350810,0.0612733590676585,
    //0.481359651258372,-0.777185751700524,0.364441894835331,0.0519458381077090,-0.0272190299170560,-0.0491371796736075,0.00380875201389062,
    //0.0149522583370482,-0.000302920514721367,-0.00188995033275946};//重构的高通滤波器系数
    int filter_len = 16;//滤波器长度

    //小波变换3级分解wavedec
    int extend = filter_len-1;
    //一级分解
    int y_len1 = sign_len+2*extend;
    double * y1 = (double *)malloc(sizeof(double)*y_len1);
    wextend(ori_sign, y1, sign_len, y_len1, extend);//信号左右沿周期扩充extend个长度，增加了2*extend长度
    int outdata_length1 = y_len1 - filter_len + 1;//卷积后的输出长度
    double * lo_arry = (double *)malloc(sizeof(double)*outdata_length1);//给卷积后的低通系数分配内存
    double * ho_arry = (double *)malloc(sizeof(double)*outdata_length1);//给卷积后的高通系数分配内存
    double * lo_arry1 = (double *)malloc(sizeof(double)*(sign_len+filter_len-1)/2);//1级低通滤波系数
    double * ho_arry1 = (double *)malloc(sizeof(double)*(sign_len+filter_len-1)/2);//1级高通滤波系数
    Conv1(y1, lo_d, lo_arry, y_len1, filter_len, outdata_length1);
    int n1 = 0;
    for(int i = 1; i < sign_len+filter_len-1; i+=2)
    {
        lo_arry1[n1] = lo_arry[i];
        n1++;
    }
    free(lo_arry);
    lo_arry = NULL;
    Conv1(y1, hi_d, ho_arry, y_len1, filter_len, outdata_length1);
    int m1 = 0;
    for(int i = 1; i < sign_len+filter_len-1; i+=2)
    {
        ho_arry1[m1] = ho_arry[i];
        m1++;
    }
    free(y1);
    y1 = NULL;
    free(ho_arry);
    ho_arry = NULL;
    free(ho_arry1);
    ho_arry1 = NULL;
    //二级分解
    outdata_length1 = (sign_len+filter_len-1)/2;
    int y_len2 = outdata_length1+2*extend;
    double * y2 = (double *)malloc(sizeof(double)*y_len2);
    wextend(lo_arry1, y2, outdata_length1, y_len2, extend);//信号左右沿周期扩充extend个长度，增加了2*extend长度
    free(lo_arry1);
    lo_arry1 = NULL;
    int outdata_length2 = y_len2 - filter_len + 1;
    lo_arry = (double *)malloc(sizeof(double)*outdata_length2);//给卷积后的低通系数分配内存
    ho_arry = (double *)malloc(sizeof(double)*outdata_length2);//给卷积后的高通系数分配内存
    double * lo_arry2 = (double *)malloc(sizeof(double)*(outdata_length1+filter_len-1)/2);//1级低通滤波系数
    double * ho_arry2 = (double *)malloc(sizeof(double)*(outdata_length1+filter_len-1)/2);//1级高通滤波系数
    Conv1(y2, lo_d, lo_arry, y_len2, filter_len, outdata_length2);
    int n2 = 0;
    for(int i = 1; i < outdata_length1+filter_len-1; i+=2)
    {
        lo_arry2[n2] = lo_arry[i];
        n2++;
    }
    free(lo_arry);
    lo_arry = NULL;
    Conv1(y2, hi_d, ho_arry, y_len2, filter_len, outdata_length2);
    int m2 = 0;
    for(int i = 1; i < outdata_length1+filter_len-1; i+=2)
    {
        ho_arry2[m2] = ho_arry[i];
        m2++;
    }
    free(y2);
    y2 = NULL;
    free(ho_arry);
    ho_arry = NULL;
    free(ho_arry2);
    ho_arry2 = NULL;
    //三级分解
    outdata_length2 = (outdata_length1+filter_len-1)/2;
    int y_len3 = outdata_length2+2*extend;
    double * y3 = (double *)malloc(sizeof(double)*y_len3);
    wextend(lo_arry2, y3, outdata_length2, y_len3, extend);//信号左右沿周期扩充extend个长度，增加了2*extend长度
    free(lo_arry2);
    lo_arry2 = NULL;
    int outdata_length3 = y_len3 - filter_len + 1;
    lo_arry = (double *)malloc(sizeof(double)*outdata_length3);//给卷积后的低通系数分配内存
    ho_arry = (double *)malloc(sizeof(double)*outdata_length3);//给卷积后的高通系数分配内存
    double * lo_arry3 = (double *)malloc(sizeof(double)*(outdata_length2+filter_len-1)/2);//1级低通滤波系数
    double * ho_arry3 = (double *)malloc(sizeof(double)*(outdata_length2+filter_len-1)/2);//1级高通滤波系数
    Conv1(y3, lo_d, lo_arry, y_len3, filter_len, outdata_length3);
    int n3 = 0;
    for(int i = 1; i < outdata_length2+filter_len-1; i+=2)
    {
        lo_arry3[n3] = lo_arry[i];
        n3++;
    }
    free(lo_arry);
    lo_arry = NULL;
    Conv1(y3, hi_d, ho_arry, y_len3, filter_len, outdata_length3);
    int m3 = 0;
    for(int i = 1; i < outdata_length2+filter_len-1; i+=2)
    {
        ho_arry3[m3] = ho_arry[i];
        m3++;
    }
    free(y3);
    y3 = NULL;
    free(ho_arry);
    ho_arry = NULL;
    free(ho_arry3);
    ho_arry3 = NULL;
    //分解之后的分解系数为[lo_arry3;ho_arry3;ho_arry2;ho_arry1]

    //小波变换重构wrcoef
    //第三级重构
    outdata_length3 = (outdata_length2+filter_len-1)/2;
    int z_len = 2 * outdata_length3 - 1;
    double * z = (double *)malloc(sizeof(double)*z_len);
    dyadup(lo_arry3,z,z_len);
    free(lo_arry3);
    lo_arry3 = NULL;
    int z_len1 = z_len+2*extend;
    double * z1 = (double *)malloc(sizeof(double)*z_len1);
    wextend_full(z,z1, z_len,z_len1, extend);
    free(z);
    z = NULL;
    int z1_out_len = z_len1 - filter_len + 1;
    double * z1_out = (double *)malloc(sizeof(double)*z1_out_len);
    Conv1(z1, lo_r, z1_out, z_len1, filter_len, z1_out_len);
    free(z1);
    z1 = NULL;
    double d1 = (z1_out_len - outdata_length2)/2;
    int frist1 = 1 + floor(d1);
    int last1 = z1_out_len - ceil(d1);
    double * wkeep_z1 = (double *)malloc(sizeof(double)*(last1-frist1+1));//在第三级重构好的
    int n4 = 0;
    for(int i = frist1; i <= last1; i++)
    {
        wkeep_z1[n4] = z1_out[i-1];
        n4++;
    }
    free(z1_out);
    z1_out = NULL;
    //第二级重构
    int w_len = 2 * outdata_length2 - 1;
    double * w = (double *)malloc(sizeof(double)*w_len);
    dyadup(wkeep_z1,w,w_len);
    free(wkeep_z1);
    wkeep_z1 = NULL;
    int w_len1 = w_len+2*extend;
    double * w1 = (double *)malloc(sizeof(double)*w_len1);
    wextend_full(w,w1, w_len,w_len1, extend);
    free(w);
    w = NULL;
    int w1_out_len = w_len1 - filter_len + 1;
    double * w1_out = (double *)malloc(sizeof(double)*w1_out_len);
    Conv1(w1, lo_r, w1_out, w_len1, filter_len, w1_out_len);
    free(w1);
    w1 = NULL;
    double d2 = (w1_out_len - outdata_length1)/2;
    int frist2 = 1 + floor(d2);
    int last2 = w1_out_len - ceil(d2);
    double * wkeep_w1 = (double *)malloc(sizeof(double)*(last2-frist2+1));
    int n5 = 0;
    for(int i = frist2; i <= last2; i++)
    {
        wkeep_w1[n5] = w1_out[i-1];
        n5++;
    }

    free(w1_out);
    w1_out = NULL;
    //第一级重构
    int v_len = 2 * outdata_length1 - 1;
    double * v = (double *)malloc(sizeof(double)*v_len);
    dyadup(wkeep_w1,v,v_len);
    free(wkeep_w1);
    wkeep_w1 = NULL;
    int v_len1 = v_len+2*extend;
    double * v1 = (double *)malloc(sizeof(double)*v_len1);
    wextend_full(v,v1, v_len,v_len1, extend);
    free(v);
    v = NULL;
    int v1_out_len = v_len1 - filter_len + 1;
    double * v1_out = (double *)malloc(sizeof(double)*v1_out_len);
    Conv1(v1, lo_r, v1_out, v_len1, filter_len, v1_out_len);
    free(v1);
    v1 = NULL;
    double d3 = (v1_out_len - sign_len)/2;
    int frist3 = 1 + floor(d3);
    int last3 = v1_out_len - ceil(d3);
    //double * wkeep_v1 = (double *)malloc(sizeof(double)*(last3-frist3+1))
    int n6 = 0;
    for(int i = frist3; i <= last3; i++)
    {
        out_sign[n6] = v1_out[i-1];
        n6++;
    }
    free(v1_out);
    v1_out = NULL;
    
}
