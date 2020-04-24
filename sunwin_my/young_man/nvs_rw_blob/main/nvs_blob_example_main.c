/*	writer:江山(jiang_gogogo)
*	chip:ESP_32
*  	MAIN:The using og NVS
*   用来，存储数组和读取数组
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"


//注意，我们可以定义多个存储空间来存储对应的值
#define STORAGE_NAMESPACE "storage"

//结构体
struct Temp_st 
{
	uint32_t temp_a;
	uint8_t * temp_str;
	char 	  temp_b;
};


/* 
*读取函数
*para1:要读取的结构体指针。
*para2:长度
 */
esp_err_t nvs_read_wifi(struct Temp_st *Temp_st, uint32_t *len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
	
	//首先一部操作打开存储空间
    // Open
	//para(空间名字（字符串），操作类型（只读还是读写），操作句柄)
    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        printf("NVS_READ：存储空间打开失败\n");
        return err;
    }

//注意这一步十分重要，因为在NVS存储已知长度类型的数据时，我们可以明确的传入已知的长度。
//但是这个地方对于我们传入的数组或者说结构体，我们不知道明确长度，于是我们采用下面的操作来获取要读取的数据的长度。

 // Read the size of memory space required for blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(nvs_handle, "Temp_st", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


//再来读取对应的值
    err = nvs_get_blob(nvs_handle, "Temp_st", Temp_st, &required_size);
    if (err ==ESP_ERR_NVS_NOT_FOUND )
    {
        printf("NVS_READ：key doesn’t exist");
        return err;
    }
    else if (err ==ESP_ERR_NVS_INVALID_HANDLE  )
    {
        printf("NVS_READ：handle has been closed or is NULL");
        return err;
    }
    else if (err ==ESP_ERR_NVS_INVALID_NAME  )
    {
        printf("NVS_READ：name doesn’t satisfy constraints ");
        return err;
    }
    else if (err ==ESP_ERR_NVS_INVALID_LENGTH  )
    {
        printf("NVS_READ：length is not sufficient to store data");
        return err;
    }
    else
    {
        printf("NVS_READ：读取成功");
    }

	//关闭句柄
    nvs_close(nvs_handle);
    return ESP_OK;
}

/* 
*存储函数
*para1:要读取的结构体指针。
*para2:长度
 */
esp_err_t nvs_write_wifi(struct Temp_st *Temp_st, uint32_t *len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    printf("NVS_WRITE：存储wifi信息\n");
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        printf("NVS_WRITE：存储空间打开失败\n");
        return err;
    }
    err = nvs_set_blob(nvs_handle, "Temp_st", Temp_st, len);
    if (err != ESP_OK)
    {
        printf("NVS_WRITE：存储空间存储失败\n");
        return err;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        printf("NVS_WRITE：存储空间提交失败\n");
        return err;
    }
    nvs_close(nvs_handle);
    return ESP_OK;
}

void app_main()
{
	uint32_t st_len = 0;
	//初始化NVS
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
	
	struct Temp_st Temp_st;
	
	nvs_write_wifi(&Temp_st, st_len);
	nvs_read_wifi(&Temp_st, &st_len);

}