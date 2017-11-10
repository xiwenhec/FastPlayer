# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

include_directories(
        ${CMAKE_SOURCE_DIR}/include
)

cmake_minimum_required(VERSION 3.4.1)

set(lib_src_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})


add_library(avcodec-57_lib SHARED IMPORTED)
set_target_properties( avcodec-57_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavcodec-57.so)

add_library(avdevice-57_lib SHARED IMPORTED)
set_target_properties( avdevice-57_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavdevice-57.so)


add_library(avfilter-6_lib SHARED IMPORTED)
set_target_properties( avfilter-6_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavfilter-6.so)

add_library(avformat-57_lib SHARED IMPORTED)
set_target_properties( avformat-57_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavformat-57.so)

add_library(avutil-55_lib SHARED IMPORTED)
set_target_properties( avutil-55_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavutil-55.so)

add_library(swresample-2_lib SHARED IMPORTED)
set_target_properties( swresample-2_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libswresample-2.so)

add_library(libyuv_lib SHARED IMPORTED)
set_target_properties( libyuv_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libyuv.so)

add_library(swscale-4_lib SHARED IMPORTED)
set_target_properties( swscale-4_lib PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libswscale-4.so)

add_library(fastplayer SHARED FastPlayer.c)

find_library(log-lib log )



target_link_libraries(
                       fastplayer
                       avcodec-57_lib
                       avdevice-57_lib
                       avfilter-6_lib
                       avformat-57_lib
                       avutil-55_lib
                       swresample-2_lib
                       swscale-4_lib
                       libyuv_lib
                       log
                       android
                        )
