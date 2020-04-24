set(COMPONENT_SRCS "ahu_utils.c"
									 "ahu_statistics.c"
									 "ahu_respirate.c"
									 "ahu_dsp.c"
									 "ahu_wavelet.c"					
									 "eemd.c"						 									 									 
                   "emd.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_REQUIRES)
register_component()

if(CONFIG_ENABLE_STATIC_TASK_CLEAN_UP_HOOK)
    target_link_libraries(${COMPONENT_TARGET} "-Wl,--wrap=vPortCleanUpTCB")
endif()
