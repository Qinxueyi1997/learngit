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
#include "ahu_utils.h"
#include "ahu_respirate.h"
#include "ahu_statistics.h"
#include "ahu_wavelet.h"
#include "ahu_dsp.h"

#define DAC_EXAMPLE_CHANNEL     CONFIG_EXAMPLE_DAC_CHANNEL
#define ADC2_EXAMPLE_CHANNEL    CONFIG_EXAMPLE_ADC2_CHANNEL//模拟转数字
void make_further_polishment(double pr_diff, double absmax, double adaptive_absmax_normal, 
														 double *p_resp_value, double *p_heart_value,
															int *p_resp_exception_counter, int *p_heart_exception_counter, 
															history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos);												
void app_main(void)
{
    int output_data=0;
    uint8_t time=0;
    uint32_t num1=0,num2=0,num3=0;
    int read_raw1, read_raw2, read_raw3;
    uint64_t data_index = 0;
    int begin1 = 0;
    int begin2 = 0;
    int Fs = 10; 
    int sample_points = 100/Fs;

    //每段信号100个数据点
    int N_seg = 100;
    //重迭点数
    int N_overlay = 30;
    //最开始的2分钟 12*10 = 2 分钟
    int adative_range = 12;
    //正常的信号能量
    double adaptive_energy_thresh = -1;
    //正常的呼吸频率
    double adaptive_resp_normal = -1; 
    //正常的心率
    double adaptive_heart_normal = -1;     
    //正常的压阻变化绝对值
    double adaptive_prdiff_normal = -1; 
    //正常的信号绝对值最大值
    double adaptive_absmax_normal = -1; 
    
    double seg_energy = 0.;
    double seg_pre_abssum = 0.;
    double resp_value = 0.;
    double heart_value = 0.;
    double pr_diff = 0.;
    double absmax = 0.;

    int adative_range_index = 0;
    
    double * median_energy_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_energy_vec, 0x00, adative_range*sizeof(double));
    
    double * median_resp_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_resp_vec, 0x00, adative_range*sizeof(double));
    
    double * median_heart_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_heart_vec, 0x00, adative_range*sizeof(double));
    
    double * median_prdiff_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_prdiff_vec, 0x00, adative_range*sizeof(double));
    
    double * median_absmax_vec = (double *)malloc(adative_range*sizeof(double));
    memset(median_absmax_vec, 0x00, adative_range*sizeof(double));
    
    //每次取100个数据点进行检测
    double * X_X = (double *)malloc(N_seg*sizeof(double));   
    double * Pr_X = (double *)malloc(N_seg*sizeof(double));   
    
    // 0 正常 1 低通气 2 睡眠暂停 3 呼吸过快 4 坠床 5 体动
    double event_type_vec;
    int log_level = 0;
    
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


    printf("start conversion.\n");       
    //===========一些大循环里用的变量=============== [
		//实际上应该用一个循环队列，因为只有数组大小为2，所以简化了
		uint8_t hist_stat_arr_size = 2;
		history_statistics hist_stat_arr[hist_stat_arr_size];
		int hist_stat_arr_rear = 0;
		history_statistics cur_stat;
		
		memset(hist_stat_arr,0x00,sizeof(hist_stat_arr));
		memset(&cur_stat,0x00,sizeof(cur_stat));
		uint64_t datanum_after_inbed = 0;
		uint8_t enable_further_judege = 1;
		int i_resp_exception_counter = 0;		
		int i_heart_exception_counter = 0;
		double	max_value_index[2] = {0.,0.};  
		enum mon_period { IDLE = 0, IN_BED, AFTER_ADAPT };
		uint8_t current_period = IDLE;
		int out_num = 0;	
	 //================================================ ]			
    while(1) {

        //vTaskDelay(32/ portTICK_PERIOD_MS);
        //vTaskDelay( 2 * portTICK_PERIOD_MS );
        event_type_vec = 0;
        vTaskDelay(18/ portTICK_PERIOD_MS);
        time++;
        r1 = adc2_get_raw( ADC2_CHANNEL_5, ADC_WIDTH_12Bit, &read_raw1);//压电
        r2 = adc2_get_raw( ADC2_CHANNEL_6, ADC_WIDTH_12Bit, &read_raw2);//压阻
        r3 = adc2_get_raw( ADC2_CHANNEL_7, ADC_WIDTH_12Bit, &read_raw3);//VREF基准
        num1=num1+read_raw1;
        num2=num2+read_raw2;
        num3=num3+read_raw3;
        if ((r1 == ESP_OK)&&(r2 == ESP_OK)&&(r3 == ESP_OK)&&(time % sample_points == 0)) {
            output_data++;
            if(current_period == IDLE && log_level < 1)
            	printf("%d      %d      %d      %d      %d  \n", current_period, output_data, read_raw1,read_raw2, read_raw3 );
            read_raw1=num1/sample_points;
            read_raw2=num2/sample_points;
            read_raw3=num3/sample_points;
            num1=0;num2=0;num3=0;                        
            
            //by xiayi, 连续10个采样点有足量压阻才算数===[          
            if(data_index == 0) {
	            if(read_raw2 < 1000) {							           	
								output_data = 0;
	            }
	            if(output_data < 10)	{     															
	            		continue;                    	            	
	            } 
          	}
          	// ]
            data_index++;                 		                    	
        }else if ( r1== ESP_ERR_INVALID_STATE ) {
        		//by xiayi
        		continue;
            //printf("%s: ADC2 not initialized yet.\n", esp_err_to_name(r1));
        } else if ( r1 == ESP_ERR_TIMEOUT ) {
            //This can not happen in this example. But if WiFi is in use, such error code could be returned.
         //   printf("%s: ADC2 is in use by Wi-Fi.\n", esp_err_to_name(r1));
            //by xiayi
            continue;
        } else {
          //  printf("%s\n", esp_err_to_name(r1));
          // by xiayi
          continue;
        }
				if(time % sample_points != 0) 
					continue;
				time = 0; 
				
        //开始的信号不用，用于获取一些自适应的参数，取前两分钟
        //printf("person in bed detected \n");
                
        current_period = IN_BED;
        if(data_index <= adative_range*N_seg){
            //printf("%llu:      PV:  %d      PR:  %d      VREF:  %d  \n", data_index, read_raw1,read_raw2, read_raw3 );
            if(current_period == IN_BED && log_level < 1)
            	printf("%d      %llu      %d      %d      %d  \n", current_period, data_index, read_raw1,read_raw2, read_raw3 );
            	
            X_X[begin1] = read_raw1 - read_raw3;
            Pr_X[begin1] = read_raw2;            
            begin1++;
            
            if(begin1==N_seg){
                begin1 = 0;
                double * out_sign = (double *)malloc(143*sizeof(double));
                int * out_levels = (int *)malloc(5*sizeof(int));
                double * a1_resp = (double *)malloc(N_seg*sizeof(double));
                double * a1_heart = (double *)malloc(N_seg*sizeof(double));
                wavedec(X_X, out_sign, out_levels, N_seg, 5);
                wrcoef(out_sign,out_levels,a1_resp);
                //重构
                for(int i = 0; i< out_levels[0]; i++){
                    out_sign[i] = 0;
                }
                for(int i = out_levels[0]+out_levels[1]+out_levels[2]; i< out_levels[0]+out_levels[1]+out_levels[2]+out_levels[3]; i++){
                    out_sign[i] = 0;
                }
                waverec(out_sign,out_levels,a1_heart);
                
                
                free(out_sign);out_sign = NULL;
                free(out_levels);out_levels = NULL;
                
                //计算信号压电能量
                median_energy_vec[adative_range_index] = accumulate1(a1_resp,N_seg,N_seg);
                //计算呼吸频率
    						median_resp_vec[adative_range_index] = obtain_resprate_with_max_possibility(a1_resp,N_seg);
                //计算心率
                median_heart_vec[adative_range_index] = obtain_heart_by_spec(a1_heart,N_seg);

    						// 计算压阻一阶导数绝对值的和
    						double * out_diff = (double *)malloc(N_seg*sizeof(double));
    						diff_fabs(Pr_X,N_seg,out_diff,&out_num);
    						median_prdiff_vec[adative_range_index] = onearray_sum(out_diff,out_num);
    						free(out_diff);out_diff = NULL;
    						    						   						
    						//计算压电信号最大值   						    						
    						max_v_interval_fabs(X_X,max_value_index,N_seg);
    						median_absmax_vec[adative_range_index] = max_value_index[0];
   						
                adative_range_index++;
                free(a1_resp);a1_resp = NULL;
                free(a1_heart);a1_heart = NULL;
            }
            if(data_index==adative_range*N_seg){
                adaptive_energy_thresh = median(median_energy_vec,adative_range);
                adaptive_resp_normal = median(median_resp_vec,adative_range);
                adaptive_heart_normal = median(median_heart_vec,adative_range);  
                adaptive_prdiff_normal = median(median_prdiff_vec,adative_range);
                adaptive_absmax_normal = median(median_absmax_vec,adative_range);               
            }
        }
				//printf("Sleep after two minutes:\n");											
        if(data_index > adative_range*N_seg){
        		current_period = AFTER_ADAPT;
        		if(current_period == AFTER_ADAPT && log_level < 1)
            	printf("%d      %llu      %d      %d      %d  \n", current_period, data_index, read_raw1,read_raw2, read_raw3 );
            X_X[begin2] = read_raw1 - read_raw3; //压电
            Pr_X[begin2] = read_raw2; //压阻
            begin2++;
            if(begin2==N_seg){
                clock_t start, finish;//计算算法的时间
                double  duration;
                start = clock();
                begin2 = 0;
                
                double * out_sign = (double *)malloc(143*sizeof(double));
                int * out_levels = (int *)malloc(5*sizeof(int));
                double * a2_resp = (double *)malloc(N_seg*sizeof(double));
                double * a2_heart = (double *)malloc(N_seg*sizeof(double));
                wavedec(X_X, out_sign, out_levels, N_seg, 5);
                wrcoef(out_sign,out_levels,a2_resp);
                
                //重构
                for(int i = 0; i< out_levels[0]; i++){
                    out_sign[i] = 0;
                }
                for(int i = out_levels[0]+out_levels[1]+out_levels[2]; i< out_levels[0]+out_levels[1]+out_levels[2]+out_levels[3]; i++){
                    out_sign[i] = 0;
                }
                waverec(out_sign,out_levels,a2_heart);
                
                free(out_sign);out_sign = NULL;
                free(out_levels);out_levels = NULL;
                //计算压阻绝对值之和
                seg_pre_abssum = onearray_sum_fabs(Pr_X,N_seg);
                //计算能量值
                seg_energy = accumulate1(a2_resp,N_seg,N_seg);
                //离床判断
                if(seg_pre_abssum < 200)
                {
                	printf("has left bed\n");
                	//做一些reset工作
                	datanum_after_inbed = 0;
                	//清理之前分配的内存
                	free(a2_resp);a2_resp = NULL;
									free(a2_heart);a2_heart = NULL;
									finish = clock();
                	duration = (double)(finish - start) / CLOCKS_PER_SEC;
                	//printf( "\n算法时间：%f seconds\n", duration );
                	continue;
                }
                //计算呼吸频率
                resp_value = obtain_resprate_with_max_possibility(a2_resp,N_seg);
                //计算心率
                heart_value = obtain_heart_by_spec(a2_heart,N_seg);
                //计算压阻一阶导数绝对值的和
                double * out_diff = (double *)malloc(N_seg*sizeof(double));

    						diff_fabs(Pr_X,N_seg,out_diff,&out_num);
    						pr_diff = onearray_sum(out_diff,out_num);
    						free(out_diff);out_diff = NULL;
    						
                //计算压电信号最大值
    						max_v_interval_fabs(X_X,max_value_index,N_seg);
    						absmax = max_value_index[0];
    						
                cur_stat.absmax = absmax;
								cur_stat.seg_energy = seg_energy;
								cur_stat.pr_diff = pr_diff;
								cur_stat.resp_value = resp_value;
								cur_stat.heart_value = heart_value;
								               	                
                if(datanum_after_inbed == 0) {
									save_history_info(hist_stat_arr, hist_stat_arr_size,&hist_stat_arr_rear, cur_stat); //do nothing             	
                } 
                else if (enable_further_judege == 1)	//根据过往信息进一步判断                
                {
                	uint8_t hist_stat_arr_front = 0;
                	if (datanum_after_inbed >= hist_stat_arr_size) //历史数组中已经有两个数据了
                		hist_stat_arr_front = 1;
                	
                	//结合上两个10秒，进一步判断结果	
                	make_further_polishment(pr_diff, absmax, adaptive_absmax_normal, 
														 &resp_value, &heart_value,
														 &i_resp_exception_counter, &i_heart_exception_counter, 
														 hist_stat_arr,hist_stat_arr_size, hist_stat_arr_front);
                  
                  cur_stat.resp_value = resp_value;
                  cur_stat.heart_value = heart_value;
                  save_history_info(hist_stat_arr, hist_stat_arr_size,&hist_stat_arr_rear, cur_stat);														 
                } 
                else
                	; //To do
                datanum_after_inbed++;   								

                //清理加输出
                free(a2_resp);a2_resp = NULL;
								free(a2_heart);a2_heart = NULL;
                printf("\n%llusecond~",(data_index - adative_range*N_seg)/10-9);
                printf("%llusecond: ",(data_index - adative_range*N_seg)/10);

                printf("resp_value = %f,heart_value = %f\n",resp_value,heart_value);
                finish = clock();
                duration = (double)(finish - start) / CLOCKS_PER_SEC;
                printf( "\n算法时间：%f seconds\n", duration );
            }
        }
    }
    free(median_energy_vec);median_energy_vec = NULL;
    free(median_resp_vec);median_resp_vec = NULL;
    free(median_heart_vec);median_heart_vec = NULL;
    free(median_prdiff_vec);median_prdiff_vec = NULL;
    free(median_absmax_vec);median_absmax_vec = NULL;
    
    free(X_X); X_X = NULL;
    free(Pr_X); Pr_X = NULL;
}

void make_further_polishment(double pr_diff, double absmax, double adaptive_absmax_normal, 
														 double *p_resp_value, double *p_heart_value,
															int *p_resp_exception_counter, int *p_heart_exception_counter,
															history_statistics hist_stat_arr[], uint8_t arr_size, uint8_t front_pos) {
	double history_resp_sum = 0.;
	double history_heart_sum = 0.;
	uint8_t field = -1;
	if(pr_diff < 1000) {
		 //======================呼吸率================================ [
		 //std函数比较费时间，暂时不用
		 //uint8_t condition_outliers = (absmax_vec(i) > 2*adaptive_absmax_normal || absmax_vec(i) - mean(X_seg) > 3*std(X_seg));
		 uint8_t condition_outliers = (absmax > 2*adaptive_absmax_normal );
     uint8_t whether_resp_abnormal = (*p_resp_value > 35 || *p_resp_value < 10);
     if(whether_resp_abnormal == 1) {//异常的呼吸率
            if(condition_outliers == 1) { //且存在异常点,放弃本次呼吸率，用过往历史    
            		field = 3;
            		history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);       
                *p_resp_value = history_resp_sum/(front_pos - 0 + 1);
            }
            else { //没有异常点
                //To do，目前也是放弃本次测量。用一个计数器累积，如果连续3次这种情况，就报告出去
                *p_resp_exception_counter = *p_resp_exception_counter + 1;
                if(*p_resp_exception_counter < 3) {
                		field = 3;
                   	history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);       
                		*p_resp_value = history_resp_sum/(front_pos - 0 + 1);
                }
                else
                {
                    ;//todo = 0;%不改变当前计算值,报告出去
                }
            }
      }
     if(whether_resp_abnormal == 0) { //正常的呼吸率,用当前和过往的做平均  
     				field = 3;
            history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);             
            *p_resp_value = (history_resp_sum + *p_resp_value)/((front_pos - 0 + 1) + 1);
            *p_resp_exception_counter = 0;
     }
     // ========================================================= ]
     
     //======================心率================================ [
     uint8_t whether_heart_abnormal = (*p_heart_value > 150 || *p_heart_value < 40);
     if(whether_heart_abnormal == 1 ) //异常的心率
     {
     		if(condition_outliers == 1) //且存在异常点, 放弃本次心率，用过往历史
     		{
     			field = 4;
          history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);       
          *p_heart_value = history_heart_sum/(front_pos - 0 + 1);
     		}
     		else  //没有异常点
     		{
     			*p_heart_exception_counter = *p_heart_exception_counter + 1;
          if(*p_heart_exception_counter < 3) {
          		field = 4;
             	history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);       
          		*p_heart_value = history_heart_sum/(front_pos - 0 + 1);
          }
          else
          {
              ;//todo = 0;%不改变当前计算值,报告出去
          } 
     		}
     }
     if(whether_heart_abnormal == 0) //正常的心率,用当前和过往的做平均
     {
         field = 4;
         history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);             
         *p_heart_value = (history_heart_sum + *p_heart_value)/((front_pos - 0 + 1) + 1);
         *p_heart_exception_counter = 0;
     }
     // ========================================================= ]
                      	
	} else { //放弃本次呼吸率、心率，用过往历史 ??  
		field = 3;//对应呼吸
		history_resp_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);  
    *p_resp_value = history_resp_sum/(front_pos - 0 + 1);
    *p_resp_exception_counter = 0;
    
    field = 4; //对应心率
    history_heart_sum = get_history_info_sum_of_some_field(hist_stat_arr, arr_size, 0, front_pos + 1, field);  
    *p_heart_value = history_heart_sum/(front_pos - 0 + 1);
     *p_heart_exception_counter = 0;
		printf("there is body movement\n");
	}
}











