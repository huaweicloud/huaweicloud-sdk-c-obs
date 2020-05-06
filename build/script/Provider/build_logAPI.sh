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
if [ $# = 0 ]; then 
    if [ $BUILD_FOR_ARM = "true" ];then
        CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld --with-pthreads
        lib_out=aarch64
    elif [ $BUILD_FOR_NDK_AARCH64 = "true" ];then
        CXXFLAGS="-fstack-protector-all -O2" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/log4cpp --host=aarch64-linux-android CC=aarch64-linux-android-gcc --with-pthreads
        lib_out=ndk-aarch64
    elif [ $BUILD_FOR_MACOS = "true" ]; then
        mkdir build
        cd build
        cmake ../ -DCMAKE_BUILD_TYPE=Release
        lib_out = macos
    else
        CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --with-pthreads
        lib_out=linux_64
    fi
elif [ $1 = "BUILD_FOR_ARM" ]; then
    CXXFLAGS="-fstack-protector-all -Wl,-z,relro,-z,now -O2" ./configure --prefix=/usr/local/log4cpp --host=aarch64-linux-gnu --build=aarch64-gnu-linux --with-gnu-ld --with-pthreads
    lib_out=aarch64
elif [ $1 = "BUILD_FOR_NDK_AARCH64" ]; then
    CXXFLAGS="-fstack-protector-all -O2" LDFLAGS="-Wl,-z,relro,-z,now" ./configure --prefix=/usr/local/log4cpp --host=aarch64-linux-android CC=aarch64-linux-android-gcc --with-pthreads
    lib_out=ndk-aarch64
elif [ $1 = "BUILD_FOR_MACOS" ]; then
    mkdir build
    cd build
    cmake ../ -DCMAKE_BUILD_TYPE=Release
    lib_out = macos
fi

make clean 
make -j16
make uninstall
make install

cd $open_src_path

mkdir -p $log4cpplib_dir

mkdir -p $logAPI_dir/../C/$lib_out
if [ "$lib_out"x = "ndk-aarch64"x ];then
cp -af /usr/local/log4cpp/lib/liblog4cpp.a $logAPI_dir/../C/$lib_out
elif [ "$lib_out"x = "macos"x ];then
cp -af $log4cpp_dir/build/liblog4cpp.a $logAPI_dir/../C/$lib_out
else
cp -af /usr/local/log4cpp/lib/liblog4cpp*.so* $logAPI_dir/../C/$lib_out
fi

echo =========build the libeSDKLogAPI.so=========
cd $logAPI_dir
make clean
if [ "$lib_out"x = "ndk-aarch64"x ];then
make -f Makefile.ndk-aarch64
elif [ "$lib_out"x = "macos"x ];then
make -f Makefile.Macos
else
make
fi

if [ "$lib_out"x = "macos"x ]; then
cp libeSDKLogAPI.dylib ../C/$lib_out -f
else
cp libeSDKLogAPI.so ../C/$lib_out -f
fi

cd $open_src_path

