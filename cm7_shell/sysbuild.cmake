cmake_minimum_required(VERSION 3.20.0)

ExternalZephyrProject_Add(
    APPLICATION cm4_rpmsg
    SOURCE_DIR  ${APP_DIR}/../cm4_rpmsg
    BOARD       mimxrt1170_evk@B/mimxrt1176/cm4
)

# CM7 depends on CM4 being built first (for the public headers)
add_dependencies(cm7_shell cm4_rpmsg)
sysbuild_add_dependencies(CONFIGURE cm7_shell cm4_rpmsg)

# Enable SECOND_CORE_MCUX on the CM7 so it includes/copies the CM4 image
set_config_bool(cm7_shell CONFIG_SECOND_CORE_MCUX 1)

# CMAKE_CURRENT_BINARY_DIR is build/_sysbuild in sysbuild context,
# so go up one level to reach build/cm4_rpmsg/...
set_config_string(cm7_shell CONFIG_SECOND_CORE_MCUX_REMOTE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/../cm4_rpmsg/zephyr/include/public"
)
message(STATUS "Sysbuild binary dir: ${CMAKE_CURRENT_BINARY_DIR}")