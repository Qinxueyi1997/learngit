# OLD MAN PROJECT
## How to use PROJECT
//  赛为项目 智慧护理床组

 (1) smartconfig 手机端配网已完成  
 (2) 路由器密码错误，擦除上次保存密码，要求重新配网，已完成  
 (3) http协议 上传呼吸，心率的信息到服务器  
 (4) 算法移植已完成
      a.心率、呼吸 2.5s计算一次，计算完成后实时上传到服务器  
      b.  采集的pv 、pr原始数据一秒上传到服务器一次  
      c. 实时上传体动、在床、离床信息  
      d.增加深睡眠 、浅睡眠状态分析//2020.9.28      
      e.增加平均心率、平均呼吸率计算//2020.9.28      
 (5) 上传人体状态信息       
      a.数据上传错误时，绿灯按10hz快速闪动四次    
 (6) 上传原始数据  
      a.数据上传错误时，绿灯按10hz快速闪动四次  

 (7) 使用手机热点或者电脑热点都有可能导致设备无法正常联网 
  （该问题可能是esp32 本身问题，路由器下配网以往测试中都能成功）  
 (8) 新加设备处于待配网时，绿灯按照1hz频率闪烁，正常工作时绿灯常亮  
 (9) 对接aliyun物联网平台，固件管理平台（已完成测试）  
     a.ota任务开启时，系统挂起get_adc_task以及http_task，只执行ota_task  
 (10) 该demo中有aliyun的api函数，使用时，必须在aliyun的sdk环境下编译  
 (11) 由于aliyun在线ota是批量升级任务，对所有设备升级都是同一固件，
因此设备编码必须在出厂前烧录在flash中，不可因为固件升级，
而影响设备编码，通过esp32分区表功能，该问题已解决。  
 (12)大批量设备在线ota未测试    
    a.ota过程中绿灯按固件接收速度持续闪烁  
    b.ota完成后绿灯按照1hz闪烁4次后绿灯常亮，
    若失败绿灯熄灭，设备重启回溯上一版本等待下一次ota.    
    c.ota过程中删除http任务，禁止上传数据  
    d.ota过程中删除adc任务，禁止采集adc数据以及运行算法
(13)增加扫描超时，进入低功耗代码     
现有问题:  
 (1)利用手机，或者电脑热点，给设备进行配网是经常存在配网不成功的情况存在，
而利用路由器在以往测试中每次都能成功  

优化完成，二值信号量释放完成后，必要延时200ms以上，给http任务足够的时间．