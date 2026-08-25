idf_component_register(
    SRCS
        "main.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        gw_core
        gw_platform
        gw_communication
        gw_runtime
)