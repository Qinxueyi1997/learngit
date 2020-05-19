ESP-IDF Gatt Server Demo
========================

This is the demo of APIs to create a GATT service by adding attributes one by one. However, this method is defined by Bluedroid and is difficult for users to use.

Hence, we also allow users to create a GATT service with an attribute table, which releases the user from adding attributes one by one. And it is recommended for users to use. For more information about this method, please refer to [gatt_server_service_table_demo](../gatt_server_service_table).

This demo creates GATT a service and then starts advertising, waiting to be connected to a GATT client. 

To test this demo, we can run the [gatt_client_demo](../gatt_client), which can scan for and connect to this demo automatically. They will start exchanging data once the GATT client has enabled the notification function of the GATT server.

Please check the [tutorial](tutorial/Gatt_Server_Example_Walkthrough.md) for more information about this example.

//实现server 循环向client 端发送数据

//增加的程序
void throughput_server_task(void *param)
{
    vTaskDelay(2000 / portTICK_PERIOD_MS);
   uint8_t indicate_data[15];
   
    while(1) {
          send_time++;
            
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                           indicate_data[i] = i+send_time;
                          if(send_time==30) send_time=0;

                        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (!can_send_notify) {
            int res = xSemaphoreTake(gatts_semaphore, portMAX_DELAY);
            assert(res == pdTRUE);
        } else {
            if (is_connecet) {
                esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_A_APP_ID].gatts_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                            gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                            sizeof(indicate_data), indicate_data, false);
            }
        }


    }
}

