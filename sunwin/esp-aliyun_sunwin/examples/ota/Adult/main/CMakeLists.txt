idf_component_register(SRCS "app_main.c"
					   INCLUDE_DIRS "../../esp-idf/components/sleep_monitoring/include/"
					   INCLUDE_DIRS "../../esp-idf/components/sleep_monitoring/gsl-1.15/include/"
					   )

add_library(sleep_monitoring STATIC IMPORTED GLOBAL)
set_property(TARGET sleep_monitoring PROPERTY IMPORTED_LOCATION "../../esp-idf/components/sleep_monitoring/libsleep_monitoring.a")
target_link_libraries(${COMPONENT_TARGET} PUBLIC sleep_monitoring)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./app_main.c)

register_component()
