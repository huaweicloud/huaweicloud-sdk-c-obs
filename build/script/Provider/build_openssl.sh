#!/bin/bash

open_src_path=`pwd`
echo_openssl_version=`echo ${openssl_version}NULL`
if [ ${echo_openssl_version} = "NULL" ]; then
  openssl_version=openssl-1.0.2r
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
if [ -z $BUILD_FOR_ARM ]; then
./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-x86_64
elif [ $BUILD_FOR_ARM = "true" ]; then
./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-aarch64
#./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ --cross-compile-prefix=aarch64-linux-gnu- linux-aarch64
fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ linux-aarch64
#./Configure threads shared --prefix=/usr/local/openssl --openssldir=/usr/local/ssl/ --cross-compile-prefix=aarch64-linux-gnu- linux-aarch64
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $openssl_lib
mkdir -p $openssl_include
mkdir -p $static_openssl_lib
cp ${open_src_path}/${openssl_dir}/libcrypto.so*  $openssl_lib
cp ${open_src_path}/${openssl_dir}/libssl.so*  $openssl_lib
cp ${open_src_path}/${openssl_dir}/include/openssl/*.h $openssl_include
cp ${open_src_path}/${openssl_dir}/*.a  $static_openssl_lib

cd $open_src_path
