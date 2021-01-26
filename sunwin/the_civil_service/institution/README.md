# SWA_MQTT OLD MAN insition application


## How to use example
该工程代码需调用esp-idf v4.0库，若使用该工程需下载esp-idf v4.0
固件烧录完成后，需要按压10S以上才会运行算法
(1)MQTT publish topic   
publish json数据格式如下，其中包含报警及服务信息  
{"product_model":"MS001","user-id":"HF102001-A","breath":0,"heart":0,"sleepFlag":3,"waring":[{"t":"long-term departure","w":1}],  "time":"2020-12-17  15:57:19"}  
(2)HTTP POST 体征数据到赛为(阿里云)服务器  
post json数据格式如下，其中包含心率、呼吸等信息  
{"deviceId":"SWA001","breathFlag":7,"breathRate":0,"breathAvg":3,"heartRate":0,"heartAvg":0,"bodyTP":1,"pushTime":"2020-12-17    15:57:22"}  
(3)HTTP POST报警信息到赛为(阿里云)服务器  
waring_string="long-term departure";//报警信息：离床超过半小时  
waring_type="Need to see";//服务信息：需要护工查看  
### Hardware Required

This example can be executed on any ESP32 board, the only required interface is WiFi and connection to internet.

### Configure the project
#define start_time 20:00//开始报警时间戳
#define stop_time 08:00//停止报警时间戳
#define LEFT_TIME_OF_OUT_BED 30//离床报警时长
make menuconfig 配置目标IP地址以及端口
### Build and Flash
make all to build project
make flash to run code on bord
(To exit the serial monitor, type ``Ctrl-]``.)


## Example Output

```　　
has left bed
current time is : 15 
within time limits ! 
体征数据上传一次..................
address2:http://up.winsleep.club/monitor.action 
http_data_type_flag:2   post_data2:{"deviceId":"SWA001","breathFlag":7,"breathRate":0,"breathAvg":3,"heartRate":0,"heartAvg":0,"bodyTP":1,"pushTime":"2020-12-17  15:53:29"}
{"code":"200","msg":"操作成功！","success":"1"}   
I (268939) HTTP_CLIENT: HTTP POST Status = 200, content_length = -1
I (268939) HTTP_CLIENT: HTTP_EVENT_DISCONNECTED
I (269159) MQTT_EXAMPLE: Note free memory: 169940 bytes
{"product_model":"MS001","user-id":"HF102001-A","breath":0,"heart":0,"sleepFlag":3,"waring":[{"t":"normal","w":0}],"time":"2020-12-17  15:53:29"} 
I (269169) MQTT_EXAMPLE: [0] Publishing...
```　　