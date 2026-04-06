cmake_minimum_required(VERSION 3.20.0)

zephyr_interface_library_named(packtag)
zephyr_include_directories(include)
zephyr_library()

zephyr_library_sources(
    src/packtag_cm4.c
    src/packtag_recipe.c
)

if(CONFIG_CPU_CORTEX_M7)
    zephyr_library_sources(src/packtag.c)
endif()

zephyr_library_link_libraries(packtag)