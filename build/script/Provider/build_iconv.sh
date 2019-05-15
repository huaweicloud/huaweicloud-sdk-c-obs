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
	if [ -z $BUILD_FOR_ARM ]; then
	./configure --prefix=/usr/local/libiconv --enable-static
	elif [ $BUILD_FOR_ARM = "true" ]; then
	./configure --prefix=/usr/local/libiconv --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    ./configure --prefix=/usr/local/libiconv --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $iconv_lib
mkdir -p $iconv_include
mkdir -p $static_iconv_lib

cp /usr/local/libiconv/lib/*.so*  $iconv_lib
cp $open_src_path/$iconv_dir/include/*.h $iconv_include
cp /usr/local/libiconv/lib/*.a $static_iconv_lib

cd $open_src_path
