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
#define EPS 0.000001
void dft(double * pr,double * pi,int n,double * fr,double * fi)
{
    double p = 6.283185306/(1.0*n);
    for(int i= 1; i <= n; i++)
    {
        fr[i-1] = 0;
        fi[i-1] = 0;
        for(int j= 1; j <= n; j++)
        {
            fr[i-1] = fr[i-1] + pr[j-1]*cos(p*(i-1)*(j-1)) + pi[j-1]*sin(p*(i-1)*(j-1));
            fi[i-1] = fi[i-1] + pr[j-1]*(-sin(p*(i-1)*(j-1))) + pi[j-1]*cos(p*(i-1)*(j-1));
        }
    }
}
void idft(double * fr,double * fi,int n,double * pr,double * pi)
{
    double p = 6.283185306/(1.0*n);
    for(int i= 1; i <= n; i++)
    {
        pr[i-1] = 0;
        pi[i-1] = 0;
        for(int j= 1; j <= n; j++)
        {
            pr[i-1] = pr[i-1] + fr[j-1]*cos(p*(i-1)*(j-1)) - fi[j-1]*sin(p*(i-1)*(j-1));
            pi[i-1] = pi[i-1] + fr[j-1]*sin(p*(i-1)*(j-1)) + fi[j-1]*cos(p*(i-1)*(j-1));
        }
        pr[i-1] = pr[i-1]/n;
        pi[i-1] = pi[i-1]/n;
    }
}

int fft(float * pr,float * pi,int len,float * fr,float * fi)
{
    int n,k,it,m,is,i,j,nv,l0;
    double p,q,s,vr,vi,poddr,poddi;
    k = (int)ceil((double)log2(len));
    n = pow(2,k);
    if(len < n){
        for(int i = len; i < n; i++){
            pr[i] = 0;
            pi[i] = 0;
        }
    }
    for (it=0; it <= n-1; it++)  //将pr[0]和pi[0]循环赋值给fr[]和fi[]
    {
        m=it;
        is=0;
        int a = m;
        for(i=0; i<=k-1; i++)
        {
            j=a/2;
            is=2*is+(a-2*j);
            a=j;
        }
        //printf("%d\n",is);
        fr[it]=pr[is];
        fi[it]=pi[is];
    }
    pr[0]=1.0;
    pi[0]=0.0;
    p=6.283185306/(1.0*n);
    pr[1]=cos(p); //将w=e^-j2pi/n用欧拉公式表示
    pi[1]=-sin(p);

    for (i=2; i<=n-1; i++)  //计算pr[]
    {
        p=pr[i-1]*pr[1];
        q=pi[i-1]*pi[1];
        s=(pr[i-1]+pi[i-1])*(pr[1]+pi[1]);
        pr[i]=p-q; pi[i]=s-p-q;
    }
    for (it=0; it<=n-2; it=it+2)
    {
        vr=fr[it];
        vi=fi[it];
        fr[it]=vr+fr[it+1];
        fi[it]=vi+fi[it+1];
        fr[it+1]=vr-fr[it+1];
        fi[it+1]=vi-fi[it+1];
    }
    m=n/2;
    nv=2;
    for (l0=k-2; l0>=0; l0--) //蝴蝶操作
    {
        m=m/2;
        nv=2*nv;
        for (it=0; it<=(m-1)*nv; it=it+nv)
          for (j=0; j<=(nv/2)-1; j++)
            {
                p=pr[m*j]*fr[it+j+nv/2];
                q=pi[m*j]*fi[it+j+nv/2];
                s=pr[m*j]+pi[m*j];
                s=s*(fr[it+j+nv/2]+fi[it+j+nv/2]);
                poddr=p-q;
                poddi=s-p-q;
                fr[it+j+nv/2]=fr[it+j]-poddr;
                fi[it+j+nv/2]=fi[it+j]-poddi;
                fr[it+j]=fr[it+j]+poddr;
                fi[it+j]=fi[it+j]+poddi;
            }
    }
    return n;
}
int ifft(float * fr,float * fi,int len,float * pr,float * pi)
{
    int n,k,it,m,is,i,j,nv,l0;
    double p,q,s,vr,vi,poddr,poddi;
    k = (int)ceil((double)log2(len));
    n = pow(2,k);
    for (it=0; it <= n-1; it++)  //将fr[0]和fi[0]循环赋值给pr[]和pi[]
    {
        m=it;
        is=0;
        int a = m;
        for(i=0; i<=k-1; i++)
        {
            j=a/2;
            is=2*is+(a-2*j);
            a=j;
        }
        pr[it]=fr[is];
        pi[it]=fi[is];
        //printf("%f\n",pr[it]);
    }
    fr[0]=1.0;
    fi[0]=0.0;
    p=6.283185306/(1.0*n);
    fr[1]=cos(p); //将w=e^j2pi/n用欧拉公式表示
    fi[1]=sin(p);

    for (i=2; i<=n-1; i++)  //计算fr[]
    {
        p=fr[i-1]*fr[1];
        q=fi[i-1]*fi[1];
        s=(fr[i-1]+fi[i-1])*(fr[1]+fi[1]);
        fr[i]=p-q; fi[i]=s-p-q;
    }
    for (it=0; it<=n-2; it=it+2)
    {
        vr=pr[it];
        vi=pi[it];
        pr[it]=vr+pr[it+1];
        pi[it]=vi+pi[it+1];
        pr[it+1]=vr-pr[it+1];
        pi[it+1]=vi-pi[it+1];
    }
    m=n/2;
    nv=2;
    for (l0=k-2; l0>=0; l0--) //蝴蝶操作
    {
        m=m/2;
        nv=2*nv;
        for (it=0; it<=(m-1)*nv; it=it+nv)
          for (j=0; j<=(nv/2)-1; j++)
            {
                p=fr[m*j]*pr[it+j+nv/2];
                q=fi[m*j]*pi[it+j+nv/2];
                s=fr[m*j]+fi[m*j];
                s=s*(pr[it+j+nv/2]+pi[it+j+nv/2]);
                poddr=p-q;
                poddi=s-p-q;
                pr[it+j+nv/2]=pr[it+j]-poddr;
                pi[it+j+nv/2]=pi[it+j]-poddi;
                pr[it+j]=pr[it+j]+poddr;
                pi[it+j]=pi[it+j]+poddi;
            }
    }
    for (int i = 0; i < n; i++){
        pr[i] = pr[i]/n;
        pi[i] = pi[i]/n;
    }
    return n;
}
//filter函数
void filter(const double* x, double* y, int xlen, double* a, double* b, int nfilt)
{
    double tmp;
    int i,j;

    //normalization
    if( (*a-1.0>EPS) || (*a-1.0<-EPS) )
    {
        tmp=*a;
        for(i=0;i<nfilt;i++)
        {
            b[i]/=tmp;
            a[i]/=tmp;
        }
    }

    memset(y,0,xlen*sizeof(double));

    a[0]=0.0;
    for(i=0;i<xlen;i++)
    {
        for(j=0;i>=j&&j<nfilt;j++)
        {
            y[i] += (b[j]*x[i-j]-a[j]*y[i-j]);
        }
    }
    a[0]=1.0;

}
void hilbert(double * sig, double * hilt_sig, int len)
{
    int k = (int)ceil((double)log2(len));
    int n = pow(2,k);
	float * pr = (float *)malloc(n*sizeof(float));//数据长度不够2的整数次幂需要补0，这里分配大一点的空间
    float * pi = (float *)malloc(n*sizeof(float));
    float * fr = (float *)malloc(n*sizeof(float));
    float * fi = (float *)malloc(n*sizeof(float));
    for (int i=0; i < len; i++){  //生成输入信号
        pr[i] = sig[i];
        pi[i] = 0;
    }
//    dft(pr,pi,len,fr,fi);
//    int n = len;
    n = fft(pr,pi,len,fr,fi);//n为补0之后的长度
    double * h = (double *)malloc(n*sizeof(double));
    memset(h,0x00,n*sizeof(double));
    h[0] = 1;
    int a = n/2;
    h[a] = 1;
    for(int i = 1; i < a; i++){
        h[i] = 2;
    }
    for(int i = 0; i < n; i++){
        fr[i] = fr[i]*h[i];
        fi[i] = fi[i]*h[i];
    }
    ifft(fr,fi,n,pr,pi);
    for (int i=0; i< len; i++)
    {
        hilt_sig[i]=sqrt(pr[i]*pr[i]+pi[i]*pi[i]);
    }
    free(pr);pr = NULL;
    free(pi);pi = NULL;
    free(fr);fr = NULL;
    free(fi);fi = NULL;
    free(h);h = NULL;
}
