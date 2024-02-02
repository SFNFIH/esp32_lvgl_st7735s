idf_component_register(SRCS "equipment.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver
                    PRIV_REQUIRES "led_strip")
