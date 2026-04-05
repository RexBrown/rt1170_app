cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cm4_rpmsg)

target_sources(app PRIVATE
    src/main.c
    src/ipc_handler.c
    src/cmd_dispatch.c
    src/debug.c
)