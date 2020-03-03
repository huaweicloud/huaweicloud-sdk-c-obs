#!/bin/bash
echo =========== compile log4cpp==================
open_src_path=`pwd`
log4cpp_dir="./../../../platform/eSDK_LogAPI_V2.1.10/log4cpp"
logAPI_dir="./../../../platform/eSDK_LogAPI_V2.1.10/eSDKLogAPI"
log4cpplib_dir=$log4cpp_dir/lib


cd $log4cpp_dir
chmod 777 configure
aclocal -I m4
autoreconf -ivf
automake -a -c
CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --with-pthreads --enable-static

make clean 
make
make install

cd $open_src_path

mkdir -p $log4cpplib_dir
cp /usr/local/log4cpp/lib/liblog4cpp*.so* $log4cpplib_dir

echo =========build the libeSDKLogAPI.so=========
cd $logAPI_dir
make clean
make -f Makefile_static
cp libeSDKLogAPI.a .../C/linux_64/

cd $open_src_path

