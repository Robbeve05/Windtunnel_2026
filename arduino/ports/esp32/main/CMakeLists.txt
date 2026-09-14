idf_component_register(
    SRCS "app_main.c" "../board_port.c"
         "../../../src/windtunnel.c" "../../../src/protocol.c"
         "../../../src/application_tasks.c"
    INCLUDE_DIRS "../../../include")
