cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cm7_shell)

if(CONFIG_INCLUDE_REMOTE_DIR)
  set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../cm4_rpmsg/zephyr)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

if(CONFIG_SECOND_CORE_MCUX)
  set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../cm4_rpmsg/zephyr)
  zephyr_include_directories(${REMOTE_ZEPHYR_DIR}/include/public)
endif()

target_sources(app PRIVATE
    src/main.c
    src/ipc_handler.c
    src/shell_cm4.c
    src/shell_ver.c
)