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
#include "sw_al.h"
#include "cJSON.h"
#define EPS 0.000001
extern int HTTP_Send_Data[20],HTTP_Body_Data[6],HTTP_MonitorSleep_Data[1],remind;
extern const char *Device_ID;
extern char *cname,*month;
double linear_regression(double *array_x,double *array_y,int size_t)
{
double a=0,b=0,sum_x=0,sum_y=0,x_ave=0,y_ave=0,lxy=0,xisub=0;;
for(int i=0;i<size_t;i++)
{
	sum_x+=array_x[i];
    	sum_y+=array_y[i];
}
x_ave=sum_x/size_t;
y_ave=sum_y/size_t;
for(int i=0;i !=size_t;i++)
{
lxy+=(array_x[i]-x_ave)*(array_y[i]-y_ave);
xisub+=(array_x[i]-x_ave)*(array_x[i]-x_ave);
}
b=lxy/xisub;
a=y_ave-b*x_ave;
printf("y=%0.2fx+%0.2f\n", b, a);
return b;
}


void Get_Current_Original_Data(char *dat)
{
     cJSON *root=NULL;//定义一个指针变量
     root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);//
    cJSON_AddStringToObject(root,"pushTime",cname);
    cJSON *array=NULL;

     cJSON_AddItemToObject(root,"original",array=cJSON_CreateArray());  
    
      cJSON *ArrayItem0=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem0,"pv",HTTP_Send_Data[0]);
      cJSON_AddNumberToObject(ArrayItem0,"pr",HTTP_Send_Data[1]);
    
      cJSON_AddItemToArray(array,ArrayItem0);
      cJSON *ArrayItem1=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem1,"pv",HTTP_Send_Data[2]);
      cJSON_AddNumberToObject(ArrayItem1,"pr",HTTP_Send_Data[3]);
   
      cJSON_AddItemToArray(array,ArrayItem1);
      cJSON *ArrayItem2=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem2,"pv",HTTP_Send_Data[4]);
      cJSON_AddNumberToObject(ArrayItem2,"pr",HTTP_Send_Data[5]);

      cJSON_AddItemToArray(array,ArrayItem2);
      cJSON *ArrayItem3=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem3,"pv",HTTP_Send_Data[6]);
      cJSON_AddNumberToObject(ArrayItem3,"pr",HTTP_Send_Data[7]);

      cJSON_AddItemToArray(array,ArrayItem3);
     cJSON *ArrayItem4=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem4,"pv",HTTP_Send_Data[8]);
      cJSON_AddNumberToObject(ArrayItem4,"pr",HTTP_Send_Data[9]);
      
      cJSON_AddItemToArray(array,ArrayItem4);
      cJSON *ArrayItem5=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem5,"pv",HTTP_Send_Data[10]);
      cJSON_AddNumberToObject(ArrayItem5,"pr",HTTP_Send_Data[11]);
  
      cJSON_AddItemToArray(array,ArrayItem5);
      cJSON *ArrayItem6=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem6,"pv",HTTP_Send_Data[12]);
      cJSON_AddNumberToObject(ArrayItem6,"pr",HTTP_Send_Data[13]);
  
      cJSON_AddItemToArray(array,ArrayItem6);
      cJSON *ArrayItem7=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem7,"pv",HTTP_Send_Data[14]);
      cJSON_AddNumberToObject(ArrayItem7,"pr",HTTP_Send_Data[15]);

      cJSON_AddItemToArray(array,ArrayItem7);
      cJSON *ArrayItem8=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem8,"pv",HTTP_Send_Data[16]);
      cJSON_AddNumberToObject(ArrayItem8,"pr",HTTP_Send_Data[17]);

      cJSON_AddItemToArray(array,ArrayItem8);
      cJSON *ArrayItem9=cJSON_CreateObject();
      cJSON_AddNumberToObject(ArrayItem9,"pv",HTTP_Send_Data[18]);
      cJSON_AddNumberToObject(ArrayItem9,"pr",HTTP_Send_Data[19]);  
      
      cJSON_AddItemToArray(array,ArrayItem9);
     char *out=cJSON_PrintUnformatted(root);
     memcpy(dat,out,strlen(out));//将整合的数据copy给dat
     cJSON_Delete(root);
     free(out);
}

void Get_Current_Body_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 成人
    cJSON_AddNumberToObject(root,"breathFlag",HTTP_Body_Data[0]);//状态标志位 1正常 2 呼吸暂停 3低通气 4 呼吸过快 5 不通气6 在床 7 离床 8 体动
    cJSON_AddNumberToObject(root,"breathRate",HTTP_Body_Data[1]);//呼吸频率
    cJSON_AddNumberToObject(root,"breathAvg",HTTP_Body_Data[2]);//呼吸平均值
    cJSON_AddNumberToObject(root,"heartRate",HTTP_Body_Data[3]);//心率
     cJSON_AddNumberToObject(root,"heartAvg",HTTP_Body_Data[4]);//心率平均值
    cJSON_AddNumberToObject(root,"bodyTP",HTTP_Body_Data[5]);//体温
    cJSON_AddStringToObject(root,"pushTime",cname);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

void Get_Current_monitorSleep_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 成人
    cJSON_AddNumberToObject(root,"sleepStatus",HTTP_MonitorSleep_Data[0]);//1深睡眠2浅睡眠3清醒
    cJSON_AddStringToObject(root,"pushTime",cname);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

void Get_Current_remind_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    root =cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceId",Device_ID);// 成人
    cJSON_AddNumberToObject(root,"warnFlag",remind);//0无，１离床
    cJSON_AddStringToObject(root,"pushTime",cname);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}

void Get_Current_abnormal_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    cJSON *array=NULL;
    cJSON *array1=NULL;
    cJSON *array2=NULL;
    root =cJSON_CreateObject();
    array =cJSON_CreateObject();
    array1 =cJSON_CreateObject();
    array2 =cJSON_CreateObject();
    cJSON_AddItemToObject(root,"deviceId",cJSON_CreateString(Device_ID)); 
    cJSON_AddItemToObject(root,"abnormal",array1);
    cJSON_AddStringToObject(array1,"type","timeout_off_bed");
    cJSON_AddStringToObject(array1,"time",cname);
    cJSON_AddStringToObject(root,"time",month);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}
void Get_Current_abnormal_service_Data(char *dat)
{
    cJSON *root=NULL;//定义一个指针变量
    cJSON *array=NULL;
    cJSON *array1=NULL;
    cJSON *array2=NULL;
    root =cJSON_CreateObject();
    array =cJSON_CreateObject();
    array1 =cJSON_CreateObject();
    array2 =cJSON_CreateObject();
    cJSON_AddItemToObject(root,"user-id",cJSON_CreateString("HF102001-A")); 
    cJSON_AddItemToObject(root,"abnormal",array1);
    cJSON_AddStringToObject(array1,"type","timeout_off_bed");
    cJSON_AddStringToObject(array1,"time",cname);
    cJSON_AddItemToObject(root,"service",array2);
    cJSON_AddStringToObject(array2,"type","timeout_off_bed");
    cJSON_AddStringToObject(array2,"time",cname);
    cJSON_AddStringToObject(root,"time",month);
    char *out=cJSON_PrintUnformatted(root);
    memcpy(dat,out,strlen(out));//将整合的数据copy给dat
    cJSON_Delete(root);
    free(out);
}