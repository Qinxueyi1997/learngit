# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/main/ahu_sleep/include $(PROJECT_PATH)/main/ahu_sleep/gsl-1.15/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/main -lmain $(PROJECT_PATH)/main/ahu_sleep/libsleep_monitoring.a
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += main
COMPONENT_LDFRAGMENTS += 
component-main-build: 
