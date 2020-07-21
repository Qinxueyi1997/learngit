# OLD MAN PROJECT
## How to use PROJECT
//  赛为项目 智能护理床组

（1）smartconfig 手机端配网已完成
（2）路由器密码错误，擦除上次保存密码，要求重新配网，已完成
（3）http 上传呼吸，心率的信息
（4）算法移植已完成
   (5)   上传人体状态信息
   (6)   手机热点或者电脑热点都有可能导致设备无法正常联网
（7）新加待配网时，绿灯按照1hz频率闪烁
（8）数据发送失败时，按照10hz频率闪烁四次
   (9)   对接aliyun物联网平台，固件管理平台（已完成测试）
（10）该demo中有aliyun的api函数，使用时，aliyun的sdk环境下编译

Bug:
(1)利用手机，或者电脑热点，给设备进行配网是经常存在配网不成功的情况存在，
而利用路由器在以往测试中每次都能成功

### Hardware Required

#### ESP32 platform

* A development board with ESP32 SoC (e.g., ESP32-DevKitC, ESP-WROVER-KIT, etc.)
* A USB cable for power supply and programming

We use ADC1_CHANNEL_7 (GPIO27) and DAC_CHANNEL_1 (GPIO25) by default, you need to short the two GPIOs (if you have changed the ADC2 and DAC channel, please refer to Chapter 4.11 of the `ESP32 Technical Reference Manual` to get the pin number).

#### ESP32-S2 platform

* A development board with ESP32S2BETA SoC
* A USB cable for power supply and programming

We use ADC1_CHANNEL_7 (GPIO18) and DAC_CHANNEL_1 (GPIO17) by default, you need to short the two GPIOs (if you have changed the ADC2 and DAC channel, please refer to the `ESP32S2 Technical Reference Manual` to get the pin number).

### Configure the project

```
idf.py menuconfig
```

* Set serial port under Serial Flasher Options.
* Set ADC2 and DAC channel in "Example Configuration"

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

Running this example, you will see the following log output on the serial monitor:

### ESP32 platform

```
ADC channel 7 @ GPIO 27, DAC channel 1 @ GPIO 25.
adc2_init...
start conversion.
1: 150
2: 203
3: 250
4: 300
5: 351
6: 400
7: 441
8: 491
9: 547
10: 595
...
```

#### ESP32-S2 platform

```
ADC channel 7 @ GPIO 18, DAC channel 1 @ GPIO 17.
adc2_init...
start conversion.
1: 150
2: 203
3: 250
4: 300
5: 351
6: 400
7: 441
8: 491
9: 547
10: 595
...
```

## Troubleshooting

* program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
