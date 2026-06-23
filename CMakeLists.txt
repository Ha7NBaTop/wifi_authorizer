cmake_minimum_required(VERSION 3.10)

project(wifi_authorizer C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBNL REQUIRED
    libnl-3.0
    libnl-genl-3.0
)

add_executable(main main.c
        monitor.c)

target_include_directories(main PRIVATE ${LIBNL_INCLUDE_DIRS})
target_compile_options(main PRIVATE ${LIBNL_CFLAGS_OTHER})
target_link_directories(main PRIVATE ${LIBNL_LIBRARY_DIRS})
target_link_libraries(main PRIVATE ${LIBNL_LIBRARIES})