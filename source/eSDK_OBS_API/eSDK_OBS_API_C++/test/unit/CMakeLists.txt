cmake_minimum_required(VERSION 3.10)
project(obs_ssl_unit_tests C)

# Use C99 because the unit-test sources and the SDK C sources rely on it.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Dependency selection strategy:
# - On Linux, prefer the OpenSSL/libcurl artifacts that already exist inside this repository.
# - On non-Linux platforms, fall back to pkg-config discovery.
#
# This keeps the Linux build closer to the main SDK build layout and avoids relying on
# system-installed OpenSSL/libcurl versions that may differ from the repository copy.
set(UT_OPENSSL_INCLUDE_DIRS "")
set(UT_LIBCURL_INCLUDE_DIRS "")
set(UT_LIBRARY_DIRS "")
set(UT_OPENSSL_LIBRARIES "")
set(UT_LIBCURL_LIBRARIES "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # test/unit -> test -> eSDK_OBS_API_C++ -> eSDK_OBS_API -> source -> repo root
    get_filename_component(UT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

    # Reuse the same repository layout used by the main SDK build:
    # - x86_64 maps to build/linux
    # - aarch64 maps to build/arm
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(UT_PROVIDER_ARCH "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(UT_PROVIDER_ARCH "linux")
    else()
        message(FATAL_ERROR "Unsupported Linux architecture for unit tests: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    set(UT_PROVIDER_ROOT "${UT_REPO_ROOT}/build/script/Provider/build/${UT_PROVIDER_ARCH}")

    # These cache variables allow manual override if your local repository uses a custom path.
    set(UT_OPENSSL_ROOT "${UT_PROVIDER_ROOT}/tongsuo-8.4.0" CACHE PATH "Repository OpenSSL root directory for unit tests")
    set(UT_LIBCURL_ROOT "${UT_PROVIDER_ROOT}/curl-v2025.3.9-SM" CACHE PATH "Repository libcurl root directory for unit tests")

    if(NOT EXISTS "${UT_OPENSSL_ROOT}/include/openssl/ssl.h")
        message(FATAL_ERROR "OpenSSL headers not found in repository path: ${UT_OPENSSL_ROOT}/include")
    endif()

    if(NOT EXISTS "${UT_OPENSSL_ROOT}/lib/libssl.so")
        message(FATAL_ERROR "OpenSSL library not found in repository path: ${UT_OPENSSL_ROOT}/lib")
    endif()

    if(NOT EXISTS "${UT_LIBCURL_ROOT}/include/curl/curl.h")
        message(FATAL_ERROR "libcurl headers not found in repository path: ${UT_LIBCURL_ROOT}/include")
    endif()

    if(NOT EXISTS "${UT_LIBCURL_ROOT}/lib/libcurl.so")
        message(FATAL_ERROR "libcurl library not found in repository path: ${UT_LIBCURL_ROOT}/lib")
    endif()

    set(UT_OPENSSL_INCLUDE_DIRS "${UT_OPENSSL_ROOT}/include")
    set(UT_LIBCURL_INCLUDE_DIRS "${UT_LIBCURL_ROOT}/include")
    set(UT_LIBRARY_DIRS
        "${UT_OPENSSL_ROOT}/lib"
        "${UT_LIBCURL_ROOT}/lib"
    )
    set(UT_OPENSSL_LIBRARIES
        "${UT_OPENSSL_ROOT}/lib/libssl.so"
        "${UT_OPENSSL_ROOT}/lib/libcrypto.so"
    )
    set(UT_LIBCURL_LIBRARIES
        "${UT_LIBCURL_ROOT}/lib/libcurl.so"
    )
else()
    # For macOS or other non-Linux environments, continue to use pkg-config.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OPENSSL REQUIRED openssl)
    pkg_check_modules(LIBCURL REQUIRED libcurl)

    set(UT_OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
    set(UT_LIBCURL_INCLUDE_DIRS ${LIBCURL_INCLUDE_DIRS})
    set(UT_LIBRARY_DIRS
        ${OPENSSL_LIBRARY_DIRS}
        ${LIBCURL_LIBRARY_DIRS}
    )
    set(UT_OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES})
    set(UT_LIBCURL_LIBRARIES ${LIBCURL_LIBRARIES})
endif()

# Header search paths:
# 1. OpenSSL/libcurl headers selected above
# 2. Unit-test helper headers in this directory
# 3. SDK headers and source-local headers used by the real implementation
include_directories(
    ${UT_OPENSSL_INCLUDE_DIRS}
    ${UT_LIBCURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Extra library search paths.
# On Linux these point to the repository's own OpenSSL/libcurl copies.
# On non-Linux they come from pkg-config.
link_directories(
    ${UT_LIBRARY_DIRS}
)

# Global compile definitions used by the unit-test target.
# - UNIT_TEST: marks this build as a unit-test build
# - _GNU_SOURCE: keeps GNU extensions enabled where the SDK expects them
# - LIBOBS_VER_*: provides version macros that request.c uses during compilation
add_definitions(
    -DUNIT_TEST
    -D_GNU_SOURCE
)

# Common compile flags for the unit-test build.
# -Wall/-Wextra help surface potential issues
# -ffunction-sections/-fdata-sections allow the linker to drop unused code paths
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections")

add_executable(obs_ssl_unit_tests
    main.c
    src/test_framework.c
    src/test_mocks.c
    src/real_impl.c
    tests/test_setup_mtls.c
    tests/test_setup_ca.c
    tests/test_password_callback.c
)

# Platform-specific linker flags:
# - macOS uses -dead_strip to remove unused code
# - Linux/other Unix-like systems commonly use --gc-sections for the same purpose
#
# We keep the handling explicit so beginners can clearly see which branch applies
# to macOS and which branch applies to Linux.
set(UT_LINK_FLAGS "")

if(APPLE)
    set(UT_LINK_FLAGS "-Wl,-dead_strip")
elseif(UNIX)
    set(UT_LINK_FLAGS "-Wl,--gc-sections")
endif()

if(UT_LINK_FLAGS)
    set_target_properties(obs_ssl_unit_tests PROPERTIES LINK_FLAGS "${UT_LINK_FLAGS}")
endif()

# When Linux uses repository libraries, runtime lookup also needs those library directories.
# BUILD_RPATH tells the executable where to look for libssl/libcrypto/libcurl at runtime.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(obs_ssl_unit_tests PROPERTIES BUILD_RPATH "${UT_LIBRARY_DIRS}")
endif()

# Link against the selected OpenSSL/libcurl libraries.
target_link_libraries(obs_ssl_unit_tests
    ${UT_OPENSSL_LIBRARIES}
    ${UT_LIBCURL_LIBRARIES}
)

# Register this executable as a ctest test target.
enable_testing()
add_test(NAME obs_ssl_unit_tests COMMAND obs_ssl_unit_tests)