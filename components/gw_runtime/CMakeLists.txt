idf_component_register(
    SRCS
        "src/gw_runtime.c"
        "src/gw_node_manager.c"
        "src/gw_command_service.c"
        "src/gw_event_service.c"
        "src/gw_sim_test.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        gw_core
        gw_communication
        gw_protocol
)
