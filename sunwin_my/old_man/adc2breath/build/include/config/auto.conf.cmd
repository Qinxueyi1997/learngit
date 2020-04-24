deps_config := \
	D:/ESP32/esp/esp-idf/components/app_trace/Kconfig \
	D:/ESP32/esp/esp-idf/components/aws_iot/Kconfig \
	D:/ESP32/esp/esp-idf/components/bt/Kconfig \
	D:/ESP32/esp/esp-idf/components/driver/Kconfig \
	D:/ESP32/esp/esp-idf/components/efuse/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp32/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp_adc_cal/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp_event/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp_http_client/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp_http_server/Kconfig \
	D:/ESP32/esp/esp-idf/components/esp_https_ota/Kconfig \
	D:/ESP32/esp/esp-idf/components/espcoredump/Kconfig \
	D:/ESP32/esp/esp-idf/components/ethernet/Kconfig \
	D:/ESP32/esp/esp-idf/components/fatfs/Kconfig \
	D:/ESP32/esp/esp-idf/components/freemodbus/Kconfig \
	D:/ESP32/esp/esp-idf/components/freertos/Kconfig \
	D:/ESP32/esp/esp-idf/components/heap/Kconfig \
	D:/ESP32/esp/esp-idf/components/libsodium/Kconfig \
	D:/ESP32/esp/esp-idf/components/log/Kconfig \
	D:/ESP32/esp/esp-idf/components/lwip/Kconfig \
	D:/ESP32/esp/esp-idf/components/mbedtls/Kconfig \
	D:/ESP32/esp/esp-idf/components/mdns/Kconfig \
	D:/ESP32/esp/esp-idf/components/mqtt/Kconfig \
	D:/ESP32/esp/esp-idf/components/nvs_flash/Kconfig \
	D:/ESP32/esp/esp-idf/components/openssl/Kconfig \
	D:/ESP32/esp/esp-idf/components/pthread/Kconfig \
	D:/ESP32/esp/esp-idf/components/spi_flash/Kconfig \
	D:/ESP32/esp/esp-idf/components/spiffs/Kconfig \
	D:/ESP32/esp/esp-idf/components/tcpip_adapter/Kconfig \
	D:/ESP32/esp/esp-idf/components/unity/Kconfig \
	D:/ESP32/esp/esp-idf/components/vfs/Kconfig \
	D:/ESP32/esp/esp-idf/components/wear_levelling/Kconfig \
	D:/ESP32/esp/esp-idf/components/wifi_provisioning/Kconfig \
	D:/ESP32/esp/esp-idf/components/app_update/Kconfig.projbuild \
	D:/ESP32/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	D:/ESP32/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	D:/adc2/main/Kconfig.projbuild \
	D:/ESP32/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/d/ESP32/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_TARGET)" "esp32"
include/config/auto.conf: FORCE
endif
ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
