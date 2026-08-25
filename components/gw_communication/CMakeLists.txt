idf_component_register(
    SRCS
        "src/gw_communication.c"
        "src/gw_transport.c"


    INCLUDE_DIRS
        "include"

    REQUIRES
        gw_core
        gw_platform
        gw_protocol
)
