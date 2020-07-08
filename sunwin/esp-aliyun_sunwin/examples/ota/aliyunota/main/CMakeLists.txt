idf_component_register(SRCS "adc1_main.c"
                    INCLUDE_DIRS ".")
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./app_main.c)

register_component()
