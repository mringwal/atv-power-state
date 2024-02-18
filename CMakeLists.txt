cmake_minimum_required(VERSION 3.27)
project(atv_power_state C)

set(CMAKE_C_STANDARD 11)

# fink pkgconfig
find_package(PkgConfig REQUIRED)

# libusb
pkg_check_modules(LIBIMOBILEDEVICE REQUIRED libimobiledevice-1.0)
include_directories(${LIBIMOBILEDEVICE_INCLUDE_DIRS})
link_directories(${LIBIMOBILEDEVICE_LIBRARY_DIRS})
link_libraries(${LIBIMOBILEDEVICE_LIBRARIES})

# libcurl
pkg_check_modules(LIBCURL libcurl)
if(LIBCURL_FOUND)
    include_directories(${LIBCURL_INCLUDE_DIRS})
    link_directories(${LIBCURL_LIBRARY_DIRS})
    link_libraries(${LIBCURL_LIBRARIES})
    add_compile_definitions(HAVE_LIBCURL)
endif ()

add_executable(atv-listener atv.c atv-listener.c)
