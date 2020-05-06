#!/bin/bash
echo =========== compile curl ==================
open_src_path=`pwd`
if [ "NULL"${curl_version} = "NULL" ]; then
   curl_version=curl-7.66.0
fi
curl_dir=./../../../third_party_groupware/eSDK_Storage_Plugins/${curl_version}
curl_lib=`pwd`/build/${curl_version}/lib
curl_include=`pwd`/build/${curl_version}/include/curl
static_curl_lib=`pwd`/build/${curl_version}/static_package/lib

#nghttp2_dir=/usr/local/nghttp2/lib
#rm -rf  $nghttp2_dir/libnghttp2.so $nghttp2_dir/libnghttp2.so.14

cd $curl_dir
chmod 777 configure

if [ $# = 0 ]; then 
    if [ $BUILD_FOR_ARM = "true" ];then
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
        lib_out="aarch64"
    elif [ $BUILD_FOR_NDK_AARCH64 = "true" ];then
        CFLAGS="-fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl --host=aarch64-linux-android CC=aarch64-linux-android-gcc
        lib_out="ndk-aarch64"
    elif [ $BUILD_FOR_MACOS = "true" ];then
        CFLAGS="-fstack-protector-all" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl
        lib_out="macos"
    else
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl #--with-nghttp2=/usr/local/nghttp2
        lib_out="linux_x64"
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
    lib_out="aarch64"
elif [ $1 = "BUILD_FOR_NDK_AARCH64" ]; then
    CFLAGS="-fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl --host=aarch64-linux-android CC=aarch64-linux-android-gcc
    lib_out="ndk-aarch64"
elif [ $1 = "BUILD_FOR_MACOS" ]; then
    CFLAGS="-fstack-protector-all" ./configure --prefix=/usr/local/curl --with-ssl=/usr/local/openssl
    lib_out="macos"
fi

#ln -s /usr/local/nghttp2/lib/libnghttp2.so.14.16.2 /usr/local/nghttp2/lib/libnghttp2.so
#ln -s /usr/local/nghttp2/lib/libnghttp2.so.14.16.2 /usr/local/nghttp2/lib/libnghttp2.so.14

make clean 
make
make install

cd $open_src_path
mkdir -p $curl_lib
mkdir -p $curl_include
mkdir -p $static_curl_lib
if [ $1 = "BUILD_FOR_MACOS" ]; then
cp -a $open_src_path/$curl_dir/lib/.libs/*.dylib  $curl_lib
else
cp $open_src_path/$curl_dir/lib/.libs/libcurl.so*  $curl_lib
fi
cp $open_src_path/$curl_dir/include/curl/*.h $curl_include
cp $open_src_path/$curl_dir/lib/.libs/*.a  $static_curl_lib

cd $open_src_path
