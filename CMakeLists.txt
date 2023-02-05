cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0004 NEW)
 
project(huaweicloud-obs-c-sdk)
 
#***********************************************************************
#*
#*  basic definitions
#*
#***********************************************************************
add_definitions(-D__LINUX_USR__)
add_definitions(-D__USE_GNU)
add_definitions(-D__POSIX_SOURCE)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DV5R1C00)
add_definitions(-DMY_PID=196)
add_definitions(-D__DPAX_LINUX_USR__)
add_definitions(-D__DPAX_THREAD_NOPREEMPT__)

set(LIBOBS_VER_MAJOR "3.23")
set(LIBOBS_VER_MINOR "1")
if(NOT CMAKE_HOST_WIN32)
    add_definitions(-DLIBOBS_VER_MAJOR="${LIBOBS_VER_MAJOR}")
    add_definitions(-DLIBOBS_VER_MINOR="${LIBOBS_VER_MINOR}")
endif()

option(USE_CUSTOM "option for custom library path" OFF)
set(SECUREC_INC_DIR ${PROJECT_SOURCE_DIR}/platform/huaweisecurec/include)
set(CURL_VERSION curl-7.78.0)
set(OPENSSL_VERSION openssl-1.1.1k)
set(LIBXML2_VERSION libxml2-2.9.9)
set(PCRE_VERSION pcre-8.45)
set(ICONV_VERSION iconv-1.15)
set(CJSON_VERSION cjson-1.7.15)



execute_process(COMMAND uname -m OUTPUT_VARIABLE OARCH)
    string(STRIP "${OARCH}" ARCH)
    message(">>> ARCH ${ARCH}")
    if (${ARCH} STREQUAL "aarch64")
        set(ESDK_LOG_INC_DIR ${PROJECT_SOURCE_DIR}/platform/eSDK_LogAPI_V2.1.10)
        set(ESDK_LOG_LIB_DIR ${PROJECT_SOURCE_DIR}/platform/eSDK_LogAPI_V2.1.10/C/aarch64)
        set(LINKARCH "arm")
        #execute_process(COMMAND make -f  ${PROJECT_SOURCE_DIR}/platform/huaweisecurec/src/Makefile.aarch64)
    elseif(${ARCH} STREQUAL "x86_64")
        set(ESDK_LOG_INC_DIR ${PROJECT_SOURCE_DIR}/platform/eSDK_LogAPI_V2.1.10)
        set(ESDK_LOG_LIB_DIR ${PROJECT_SOURCE_DIR}/platform/eSDK_LogAPI_V2.1.10/C/linux_64)
        set(LINKARCH "linux")
        #execute_process(COMMAND make -f  ${PROJECT_SOURCE_DIR}/platform/huaweisecurec/src/Makefile)
    endif()

    if(NOT USE_CUSTOM)
        set(CURL_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${CURL_VERSION}/include)
        set(CURL_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${CURL_VERSION}/lib)
        set(OPENSSL_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${OPENSSL_VERSION}/include)
        set(OPENSSL_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${OPENSSL_VERSION}/lib)
        set(LIBXML2_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${LIBXML2_VERSION}/include)
        set(LIBXML2_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${LIBXML2_VERSION}/lib)
        set(PCRE_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${PCRE_VERSION}/include/pcre)
        set(PCRE_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${PCRE_VERSION}/lib)
        set(ICONV_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${ICONV_VERSION}/include)
        set(ICONV_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${ICONV_VERSION}/lib)
        set(CJSON_INC_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${CJSON_VERSION}/include)
        set(CJSON_LIB_DIR ${PROJECT_SOURCE_DIR}/build/script/Provider/build/${LINKARCH}/${CJSON_VERSION}/lib)
	set(SECUREC_LIB_DIR ${PROJECT_SOURCE_DIR}/platform/huaweisecurec/lib/${LINKARCH})
    else()
        if(NOT DEFINED CURL_INC_DIR)
            set(CURL_INC_DIR "/usr/include" CACHE STRING "")
        endif()
        if(NOT DEFINED CURL_LIB_DIR)
            set(CURL_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED OPENSSL_INC_DIR)
            set(OPENSSL_INC_DIR "/usr/include" CACHE STRING "")
        endif()
        if(NOT DEFINED OPENSSL_LIB_DIR)
            set(OPENSSL_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED LIBXML2_INC_DIR)
            set(LIBXML2_INC_DIR "/usr/include/libxml2" CACHE STRING "")
        endif()
        if(NOT DEFINED LIBXML2_LIB_DIR)
            set(LIBXML2_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED PCRE_INC_DIR)
            set(PCRE_INC_DIR "/usr/include" CACHE STRING "")
        endif()
        if(NOT DEFINED PCRE_LIB_DIR)
            set(PCRE_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED ICONV_INC_DIR)
            set(ICONV_INC_DIR "/usr/include" CACHE STRING "")
        endif()
        if(NOT DEFINED ICONV_LIB_DIR)
            set(ICONV_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED CJSON_INC_DIR)
            set(ICONV_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()
        if(NOT DEFINED CJSON_LIB_DIR)
            set(ICONV_LIB_DIR "/usr/lib64" CACHE STRING "")
        endif()        
    endif()

message(STATUS "CURL_INC_DIR: ${CURL_INC_DIR}")
message(STATUS "CURL_LIB_DIR: ${CURL_LIB_DIR}")
message(STATUS "OPENSSL_INC_DIR: ${OPENSSL_INC_DIR}")
message(STATUS "OPENSSL_LIB_DIR: ${OPENSSL_LIB_DIR}")
message(STATUS "LIBXML2_INC_DIR: ${LIBXML2_INC_DIR}")
message(STATUS "LIBXML2_LIB_DIR: ${LIBXML2_LIB_DIR}")
message(STATUS "PCRE_INC_DIR: ${PCRE_INC_DIR}")
message(STATUS "PCRE_LIB_DIR: ${PCRE_LIB_DIR}")
message(STATUS "ICONV_INC_DIR: ${ICONV_INC_DIR}")
message(STATUS "ICONV_LIB_DIR: ${ICONV_LIB_DIR}")
message(STATUS "CJSON_INC_DIR: ${CJSON_INC_DIR}")
message(STATUS "CJSON_LIB_DIR: ${CJSON_LIB_DIR}")

#***********************************************************************
#*
#*  compile mode
#*
#***********************************************************************
SET(ESDK_BASE_FLAGS            "-g -Wall -fPIC -fno-strict-overflow -s -shared")
SET(ESDK_SP_C_FLAGS            "-std=gnu99")
SET(ESDK_SP_C++_FLAGS          "-std=c++11")
SET(ESDK_SP_RELEASE_FLAGS      "-O2")
SET(ESDK_SP_DEBUG_FLAGS        "-O0 -coverage")
if(NOT CMAKE_HOST_WIN32)
    SET(SEC_REQ_FLAGS             "-fstack-protector-strong -fPIE -pie -Wl,-z,relro,-z,now,-z,noexecstack")
    SET(SEC_SUG_BASE_FLAGS        "-Wformat=2")
    SET(SEC_SUG_SP_C_FLAGS        "-Wshadow -Wstack-protector")
endif()
 
SET(ESDK_C_FLAGS     "${ESDK_BASE_FLAGS} ${ESDK_SP_C_FLAGS}")
SET(ESDK_C++_FLAGS   "${ESDK_BASE_FLAGS} ${ESDK_SP_C++_FLAGS}")
SET(SEC_C_FLAGS     "${SEC_REQ_FLAGS} ${SEC_SUG_BASE_FLAGS} ${SEC_SUG_SP_C_FLAGS}")
SET(SEC_C++_FLAGS   "${SEC_REQ_FLAGS} ${SEC_SUG_BASE_FLAGS}")
SET(CMAKE_SKIP_RPATH TRUE)
 
if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(">>> Release Mode")
    set(CMAKE_C_FLAGS     "${ESDK_C_FLAGS} ${ESDK_SP_RELEASE_FLAGS} ${SEC_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ESDK_C++_FLAGS} ${ESDK_SP_RELEASE_FLAGS} ${SEC_C++_FLAGS}")
 
else()
    message(">>> Debug Mode")
    set(CMAKE_C_FLAGS     "${ESDK_C_FLAGS} ${ESDK_SP_DEBUG_FLAGS} ${SEC_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ESDK_C++_FLAGS} ${ESDK_SP_DEBUG_FLAGS} ${SEC_C++_FLAGS}")
    set(COVERAGE_LINK   "gcov")
    add_definitions(-D_DEBUG)
 
endif()
 
message(STATUS ">>> Source Dir: " ${PROJECT_SOURCE_DIR})
message(STATUS ">>> Binary Dir: " ${PROJECT_BINARY_DIR})
message(STATUS ">>> C_FLAGS   : " ${CMAKE_C_FLAGS})
message(STATUS ">>> CXX_FLAGS : " ${CMAKE_CXX_FLAGS})
 
#***********************************************************************
#*
#*  set dir of output binary and library
#*
#***********************************************************************
set(EXECUTABLE_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH       ${PROJECT_BINARY_DIR}/lib)
 
#***********************************************************************
#*
#*  Link lib
#*
#***********************************************************************
include_directories(${CURL_INC_DIR})
include_directories(${OPENSSL_INC_DIR})
include_directories(${LIBXML2_INC_DIR})
include_directories(${PCRE_INC_DIR})
include_directories(${ICONV_INC_DIR})
include_directories(${CJSON_INC_DIR})
include_directories(${SECUREC_INC_DIR})
include_directories(${ESDK_LOG_INC_DIR})

link_directories(${CURL_LIB_DIR})
link_directories(${OPENSSL_LIB_DIR})
link_directories(${LIBXML2_LIB_DIR})
link_directories(${PCRE_LIB_DIR})
link_directories(${ICONV_LIB_DIR})
link_directories(${CJSON_LIB_DIR})
link_directories(${SECUREC_LIB_DIR})
link_directories(${ESDK_LOG_LIB_DIR})
include_directories(${CMAKE_SOURCE_DIR}/source/eSDK_OBS_API/eSDK_OBS_API_C++/inc)
include_directories(${CMAKE_SOURCE_DIR}/source/eSDK_OBS_API/eSDK_OBS_API_C++/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/source/eSDK_OBS_API/eSDK_OBS_API_C++/src SOURCE_DIR)
aux_source_directory(${CMAKE_SOURCE_DIR}/source/eSDK_OBS_API/eSDK_OBS_API_C++/src/bucket BUCKET_SOURCE_DIR)
aux_source_directory(${CMAKE_SOURCE_DIR}/source/eSDK_OBS_API/eSDK_OBS_API_C++/src/object OBJECT_SOURCE_DIR)
add_library(eSDKOBS SHARED ${SOURCE_DIR} ${BUCKET_SOURCE_DIR} ${OBJECT_SOURCE_DIR})
target_link_libraries(eSDKOBS curl ssl xml2 pcre iconv cjson securec eSDKLogAPI)

#***********************************************************************
#*
#*  Unset Cache
#*
#***********************************************************************
unset(USE_CUSTOM CACHE)
unset(CURL_INC_DIR CACHE)
unset(CURL_LIB_DIR CACHE)
unset(OPENSSL_INC_DIR CACHE)
unset(OPENSSL_LIB_DIR CACHE)
unset(LIBXML2_INC_DIR CACHE)
unset(LIBXML2_LIB_DIR CACHE)
unset(PCRE_INC_DIR CACHE)
unset(PCRE_LIB_DIR CACHE)
unset(ICONV_INC_DIR CACHE)
unset(ICONV_LIB_DIR CACHE)
unset(CJSON_INC_DIR CACHE)
unset(CJSON_LIB_DIR CACHE)

#***********************************************************************
#*
#*  add source code dir
#*
#***********************************************************************
#add_subdirectory(src)