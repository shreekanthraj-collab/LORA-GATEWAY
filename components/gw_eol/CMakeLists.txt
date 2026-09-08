idf_component_register(
    SRCS
        "src/gw_eol_protocol.c"
        "src/gw_eol_manager.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        gw_protocol
        gw_communication
)
