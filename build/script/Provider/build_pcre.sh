#!/bin/bash
echo =========== compile pcre ==================
open_src_path=`pwd`
pcre_dir="./../../../third_party_groupware/eSDK_Storage_Plugins/pcre-8.39/source"
pcre_lib=`pwd`/build/pcre-8.39/lib
pcre_include=`pwd`/build/pcre-8.39/include/pcre
static_pcre_lib=`pwd`/build/pcre-8.39/static_package/lib

cd $pcre_dir
chmod 777 configure
autoreconf -f -i
if [ $# = 0 ]; then 
    if [ $BUILD_FOR_ARM = "true" ];then
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
    elif [ $BUILD_FOR_NDK_AARCH64 = "true" ];then
        CFLAGS="-fstack-protector-all -g -O2" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-android CC=aarch64-linux-android-gcc
    elif [ $BUILD_FOR_MACOS = "true" ];then
        CFLAGS="-fstack-protector-all -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf
    else
        CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
elif [ $1 = "BUILD_FOR_NDK_AARCH64" ]; then
    CFLAGS="-fstack-protector-all -g -O2" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-android CC=aarch64-linux-android-gcc
elif [ $1 = "BUILD_FOR_MACOS" ]; then
    CFLAGS="-fstack-protector-all -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $pcre_lib
mkdir -p $pcre_include
mkdir -p $static_pcre_lib
if [ $1 = "BUILD_FOR_MACOS" ]; then
cp /usr/local/pcre/lib/*.dylib  $pcre_lib
else
cp /usr/local/pcre/lib/*.so*  $pcre_lib
fi
cp /usr/local/pcre/include/*.h $pcre_include
cp /usr/local/pcre/lib/*.a  $static_pcre_lib

cd $open_src_path
