cmake_minimum_required( VERSION 2.8.3 )

project( camera )

set( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib )

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /home/zhangixn/arm-openwrt/toolchain/bin/arm-openwrt-linux-gcc)
SET(CMAKE_CXX_COMPILER /home/zhangxin/arm-openwrt/toolchain/bin/arm-openwrt-linux-g++)


# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/
    /usr/local/include/
    )
    
# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /home/zhangxin/yxsk-lib)


add_library( camera STATIC
    src/cam.cpp
    )
target_link_libraries( camera 
    pthread
    ${CMAKE_FIND_ROOT_PATH}/libopencv_core.so
    ${CMAKE_FIND_ROOT_PATH}/libopencv_imgcodecs.so
    ${CMAKE_FIND_ROOT_PATH}/libAWIspApi.so
    ${CMAKE_FIND_ROOT_PATH}/libiniparser.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_ae.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_af.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_afs.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_awb.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_base.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_dev.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_gtm.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_ini.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_iso.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_math.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_md.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_pltm.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_rolloff.so
    ${CMAKE_FIND_ROOT_PATH}/libisp.so
    ${CMAKE_FIND_ROOT_PATH}/libmatrix.so
    )


add_executable(test_camera
    main.cpp
)
target_link_libraries(test_camera
    pthread
    ${CMAKE_FIND_ROOT_PATH}/libopencv_core.so
    ${CMAKE_FIND_ROOT_PATH}/libopencv_imgcodecs.so
    ${CMAKE_FIND_ROOT_PATH}/libAWIspApi.so
    ${CMAKE_FIND_ROOT_PATH}/libiniparser.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_ae.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_af.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_afs.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_awb.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_base.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_dev.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_gtm.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_ini.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_iso.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_math.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_md.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_pltm.so
    ${CMAKE_FIND_ROOT_PATH}/libisp_rolloff.so
    ${CMAKE_FIND_ROOT_PATH}/libisp.so
    ${CMAKE_FIND_ROOT_PATH}/libmatrix.so
    ${LIBRARY_OUTPUT_PATH}/libcamera.a
    )