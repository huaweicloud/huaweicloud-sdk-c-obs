#!/bin/bash

open_src_path=`pwd`
echo_openssl_version=`echo ${openssl_version}NULL`
if [ ${echo_openssl_version} = "NULL" ]; then
  openssl_version=openssl-1.1.1d
fi
openssl_dir=./../../../third_party_groupware/eSDK_Storage_Plugins/${openssl_version}
openssl_lib=`pwd`/build/${openssl_version}/lib
static_openssl_lib=`pwd`/build/${openssl_version}/static_package/lib
openssl_include=`pwd`/build/${openssl_version}/include/openssl

cd $openssl_dir
chmod 777 Configure
chmod 777 util/point.sh
chmod 777 util/pod2mantest

if [ $# = 0 ]; then 
    if [ $BUILD_FOR_ARM = "true" ];then
        CFLAGS="-Wall -O3 -fstack-protector-all -Wl,-z,relro,-z,now" ./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-aarch64
    elif [ $BUILD_FOR_NDK_AARCH64 = "true" ];then
        export ANDROID_NDK_HOME=/tmp/ndk-aarch64
        CFLAGS="-Wall -O3 -fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ android-arm64
    elif [ $BUILD_FOR_MACOS = "true" ];then
        CFLAGS="-Wall -O3 -fstack-protector-all" ./config threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/
    else
        CFLAGS="-Wall -O3 -fstack-protector-all -Wl,-z,relro,-z,now" ./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-x86_64
    fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    CFLAGS="-Wall -O3 -fstack-protector-all -Wl,-z,relro,-z,now" ./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-aarch64
elif [ $1 = "BUILD_FOR_NDK_AARCH64" ]; then
    export ANDROID_NDK_HOME=/tmp/ndk-aarch64
    CFLAGS="-Wall -O3 -fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ android-arm64
elif [ $1 = "BUILD_FOR_MACOS" ]; then
    CFLAGS="-Wall -O3 -fstack-protector-all" ./config threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $openssl_lib
mkdir -p $openssl_include
mkdir -p $static_openssl_lib
if [ $1 = "BUILD_FOR_MACOS" ]; then
cp -a ${open_src_path}/${openssl_dir}/*.dylib  $openssl_lib
else
cp ${open_src_path}/${openssl_dir}/libcrypto.so*  $openssl_lib
cp ${open_src_path}/${openssl_dir}/libssl.so*  $openssl_lib
fi
cp ${open_src_path}/${openssl_dir}/include/openssl/*.h $openssl_include
cp ${open_src_path}/${openssl_dir}/*.a  $static_openssl_lib

cd $open_src_path
