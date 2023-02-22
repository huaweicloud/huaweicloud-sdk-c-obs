#!/bin/bash
#Usage: build.sh packageName release|debug
#packageName ""

#bin
#include
#lib
#----------------------- variables --------------------#
#
G_CWD=`dirname $0`
pushd $G_CWD >/dev/null
G_CWD=`pwd`
popd >/dev/null

G_FILE_NAME=$0
G_BUILD_OPTION=release
G_BUILD_DIR=${G_CWD}
g_PATH=build

G_SECUREC_PATH=$G_CWD/../../../platform/huaweisecurec

#THIRTY_DIR
G_THIRTY_DIR=$G_CWD/../../../build/script/Provider
L_THIRTY_DIR=../../../build/script/Provider
G_PLATFORM=$G_CWD/../../../platform/
#----------------------- functions ---------------------#
L_PACKAGE_NAME=$1
L_PRODUCT_TYPE=`echo $2 | tr A-Z a-z`
L_PRODUCT=`echo $3 | tr A-Z a-z`

if [ "debug" == "$2" ];then
    G_BUILD_OPTION=debug
	export DEBUG=debug
fi

export openssl_version=openssl-1.1.1t
export curl_version=curl-7.82.0
export pcre_version=pcre-8.45
export iconv_version=iconv-1.15
export libxml2_version=libxml2-2.9.9
export cjson_version=cjson-1.7.15
# **************************************************************************** #
# Function Name: ifFailExitAndPrint
# Description: 
# Parameter: $1 command exec result code  $2 description
# Return: none
# **************************************************************************** #
ifFailExitAndPrint()
{
    if [ "0" != "$1" ] ;then
        echo "$2"
        exit 1
    fi
}

#----------third_party_groupware------------
#compileThirty L_THIRTY_DIR

#cd ${G_BUILD_DIR}

#----------libsecurec.so------------
#pushd $G_SECUREC_PATH/src >/dev/null
#make clean

# make
popd >/dev/null
echo start cmake

mv -f $G_CWD/../../../CMakeLists.txt $G_CWD/../../../CMakeListsBackup.txt
\cp -f $G_CWD/../../../CMakeLists_ndk_Static.txt $G_CWD/../../../CMakeLists.txt
make clean
mkdir cmake-build
cd cmake-build
mkdir cmake
cd cmake
cmake $G_CWD/../../../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=arm-linux-androideabi-gcc -DCMAKE_CXX_COMPILER=arm-linux-androideabi-g++ -D_ANDROID_AOI_=17 -D__ANDROID_API__=17
mv -f $G_CWD/../../../CMakeListsBackup.txt $G_CWD/../../../CMakeLists.txt
make 

cd ../../
#make clean
#make

if [ -d demo ];then
    rm -rf demo
fi
if [ -d include ];then
    rm -rf include
fi
if [ -d lib ];then
    rm -rf lib
fi
mkdir demo
mkdir include
mkdir lib


mkdir demo_static
mkdir include_static
mkdir lib_static

if [ "$G_BUILD_OPTION" == "debug" ];then
g_PATH=build-debug
fi



cp -f inc/eSDKOBS.h include
cp -f cmake-build/cmake/lib/*.a lib
cp -f ./../../../platform/huaweisecurec/include/* include
cp -f ./../../../platform/huaweisecurec/lib/ndk/libsecurec.a lib
cp -af ./../../../platform/eSDK_LogAPI_V2.1.10/C/ndk/libeSDKLogAPI.a lib
cp -af ./../../../platform/eSDK_LogAPI_V2.1.10/C/ndk/liblog4cpp* lib 
cp -af ./../../../build/script/Provider/build/ndk/${curl_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/ndk/${libxml2_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/ndk/${openssl_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/ndk/${pcre_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/ndk/${iconv_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/ndk/${cjson_version}/lib/* lib
cp -f ./../../../build/script/Provider/build/ndk/${cjson_version}/include/cJSON.h include
cp -f Makefile_demo_static_ndk demo/Makefile
cp -f OBS.ini lib
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/object_test.c" demo/object_test.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo.c" demo/demo.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.c" demo/demo_common.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.h" demo/demo_common.h
cp -f cert/client.pem demo/client.pem
cp -f cert/client.pem lib/client.pem
tar zcf ${L_PACKAGE_NAME}.tgz demo include lib readme.txt

rm -rf {demo,include,lib,"Log Collection Statement.txt"}
rm -rf {demo_static,include_static,lib_static,"Log Collection Statement.txt"}
#export PATH=${PATH}:/root/zq/arm-linux-androideabi/bin
#sh build_ndk_static.sh
