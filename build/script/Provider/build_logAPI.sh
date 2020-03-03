#!/bin/bash
echo =========== compile log4cpp==================
ARCH=`uname -m`
open_src_path=`pwd`
log4cpp_dir="./../../../platform/eSDK_LogAPI_V2.1.10/log4cpp"
logAPI_dir="./../../../platform/eSDK_LogAPI_V2.1.10/eSDKLogAPI"
log4cpplib_dir=$log4cpp_dir/lib


cd $log4cpp_dir
chmod 777 configure
aclocal -I m4
autoreconf -ivf
automake -a -c
if [ "$ARCH"x = "x86_64"x ];then
CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --with-pthreads
elif [ "$ARCH"x = "aarch64"x ];then
CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld --with-pthreads
fi

make clean 
make
make install

cd $open_src_path

mkdir -p $log4cpplib_dir
if [ "$ARCH"x = "x86_64"x ];then
cp -af /usr/local/log4cpp/lib/liblog4cpp*.so* $logAPI_dir/../C/linux_64
elif [ "$ARCH"x = "aarch64"x ];then
cp -af /usr/local/log4cpp/lib/liblog4cpp*.so* $logAPI_dir/../C/aarch64
fi

echo =========build the libeSDKLogAPI.so=========
cd $logAPI_dir
make clean
make

if [ "$ARCH"x = "x86_64"x ];then
mkdir -p ../C/linux_64
cp libeSDKLogAPI.so ../C/linux_64 -f
elif [ "$ARCH"x = "aarch64"x ];then
mkdir -p ../C/aarch64
cp libeSDKLogAPI.so ../C/aarch64 -f
fi

cd $open_src_path

