# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(IDF_PATH)/components/sleep_monitoring/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/sleep_monitoring -lsleep_monitoring $(IDF_PATH)/components/sleep_monitoring/sleep_monitoring/libsleep_monitoring.a
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += sleep_monitoring
COMPONENT_LDFRAGMENTS += $(IDF_PATH)/components/sleep_monitoring/linker.lf
component-sleep_monitoring-build: 
