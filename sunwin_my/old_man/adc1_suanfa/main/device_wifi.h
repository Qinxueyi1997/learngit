/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _APP_WIFI_H_
#define _APP_WIFI_H_

void app_wifi_initialise(void);
void app_wifi_wait_connected();
void nvs_read_data_from_flash(void);
void nvs_write_data_to_flash(void);
void Get_CurrentData(char *dat);
void wifi_initialise(void);

#endif
