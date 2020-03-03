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
	if [ -z $BUILD_FOR_ARM ]; then
	CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf
	elif [ $BUILD_FOR_ARM = "true" ]; then
	CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
CFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -g -O2" ./configure --prefix=/usr/local/pcre --enable-utf --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $pcre_lib
mkdir -p $pcre_include
mkdir -p $static_pcre_lib
cp /usr/local/pcre/lib/*.so*  $pcre_lib
cp /usr/local/pcre/include/*.h $pcre_include
cp /usr/local/pcre/lib/*.a  $static_pcre_lib

cd $open_src_path
