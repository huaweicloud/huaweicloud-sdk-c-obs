#!/bin/bash
echo =========== compile iconv ==================
open_src_path=`pwd`
iconv_dir="./../../../third_party_groupware/eSDK_Storage_Plugins/libiconv-1.15/source"
iconv_lib=`pwd`/build/iconv-1.15/lib
iconv_include=`pwd`/build/iconv-1.15/include
static_iconv_lib=`pwd`/build/iconv-1.15/static_package/lib

cd $iconv_dir
chmod 777 configure

if [ $# = 0 ]; then 
    if [ $BUILD_FOR_ARM = "true" ];then
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/libiconv --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
    elif [ $BUILD_FOR_NDK_AARCH64 = "true" ];then
        CFLAGS="-fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/libiconv --host=aarch64-linux-android CC=aarch64-linux-android-gcc
    elif [ $BUILD_FOR_MACOS = "true" ];then
        CFLAGS="-fstack-protector-all" ./configure --prefix=/usr/local/libiconv
    else
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/libiconv --enable-static
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/libiconv --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
elif [ $1 = "BUILD_FOR_NDK_AARCH64" ]; then
    CFLAGS="-fstack-protector-all" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/libiconv --host=aarch64-linux-android CC=aarch64-linux-android-gcc
elif [ $1 = "BUILD_FOR_MACOS" ]; then
    CFLAGS="-fstack-protector-all" ./configure --prefix=/usr/local/libiconv
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $iconv_lib
mkdir -p $iconv_include
mkdir -p $static_iconv_lib

if [ $1 = "BUILD_FOR_MACOS" ]; then
cp /usr/local/libiconv/lib/*.dylib  $iconv_lib
else
cp /usr/local/libiconv/lib/*.so*  $iconv_lib
fi
cp $open_src_path/$iconv_dir/include/*.h $iconv_include
cp /usr/local/libiconv/lib/*.a $static_iconv_lib

cd $open_src_path
