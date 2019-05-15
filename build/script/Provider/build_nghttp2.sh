#!/bin/bash
echo =========== compile nghttp2 ==================
open_src_path=`pwd`
if [ "NULL"${nghttp2_version} = "NULL" ]; then
   nghttp2_version=nghttp2-1.32.0
fi
nghttp2_dir=./../../../third_party_groupware/eSDK_Storage_Plugins/${nghttp2_version}
nghttp2_lib=`pwd`/build/${nghttp2_version}/lib
nghttp2_include=`pwd`/build/${nghttp2_version}/include/nghttp2
static_nghttp2_lib=`pwd`/build/${nghttp2_version}/static_package/lib

cd $nghttp2_dir
chmod 777 configure

if [ $# = 0 ]; then 
	if [ -z $BUILD_FOR_ARM ]; then
	./configure --prefix=/usr/local/nghttp2 --enable-lib-only
	elif [ $BUILD_FOR_ARM = "true" ]; then
	./configure --prefix=/usr/local/nghttp2  --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
	fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
./configure --prefix=/usr/local/nghttp2 --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld
fi

make clean 
make
make install

cd $open_src_path
mkdir -p $nghttp2_lib
mkdir -p $nghttp2_include
mkdir -p $static_nghttp2_lib
cp $open_src_path/$nghttp2_dir/lib/.libs/libnghttp2.so*  $nghttp2_lib
cp $open_src_path/$nghttp2_dir/lib/includes/nghttp2/*.h $nghttp2_include
cp $open_src_path/$nghttp2_dir/lib/.libs/*.a  $static_nghttp2_lib

cd $open_src_path
