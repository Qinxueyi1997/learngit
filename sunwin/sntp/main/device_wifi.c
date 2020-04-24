/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_wifi.h"
#include <sys/socket.h>
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "device_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "WIFI";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

void nvs_read_data_from_flash(void)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    wifi_config_t wifi_config_stored;
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    uint32_t len = sizeof(wifi_config_stored);
    ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA1, &wifi_config_stored, &len) );
    printf("[data1]: ssid:%s passwd:%s\r\n", wifi_config_stored.sta.ssid, wifi_config_stored.sta.password);
    nvs_close(handle);
}
void nvs_write_data_to_flash(void)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";

    static const char *DATA1= "param 1";

    wifi_config_t wifi_config_to_store = {
        .sta = {
            .ssid = "00000000",
          .password = "000000000000",
       },
   };

    //printf("set size:%u\r\n", sizeof(wifi_config_to_store));
    ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK( nvs_set_blob( handle, DATA1, &wifi_config_to_store, sizeof(wifi_config_to_store)) );

    ESP_ERROR_CHECK( nvs_commit(handle) );
    nvs_close(handle);

}
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void app_wifi_initialise(void)
{   
     //ESP_ERROR_CHECK(esp_netif_init());
    tcpip_adapter_init();//tcp协议初始化
    wifi_event_group = xEventGroupCreate();//创建一个事件组
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));//wifi事件
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
     wifi_config_t wifi_config = {
        .sta = {
              .ssid ="sunwin",
              .password = "sunwin2015",//设置要链接的wifi
        },
      };
   
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//设置模式
   ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));//设置wifi链接参数
  ESP_ERROR_CHECK(esp_wifi_start());//启动wifi


}

void app_wifi_wait_connected()
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}
