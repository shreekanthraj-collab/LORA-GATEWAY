idf_component_register(
    SRCS
        "src/gw_communication.c"
        "src/gw_transport.c"
        "src/gw_protocol_codec.c"
        "src/gw_node_command.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        gw_core
        gw_platform
        gw_protocol
)
