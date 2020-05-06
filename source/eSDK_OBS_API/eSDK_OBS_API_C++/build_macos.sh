#!/bin/bash
#Usage: build.sh packageName release|debug
#packageName 为""时 不打包

# ├─bin
# ├─include
# └─lib
#----------------------- variables --------------------#
#当前脚本所在路径
G_CWD=`dirname $0`
pushd $G_CWD >/dev/null
G_CWD=`pwd`
popd >/dev/null

G_FILE_NAME=$0
G_BUILD_OPTION=release
G_BUILD_DIR=${G_CWD}
g_PATH=build

G_SECUREC_PATH=$G_CWD/../../../platform/huaweisecurec

#THIRTY_DIR目录
G_THIRTY_DIR=$G_CWD/../../../build/script/Provider
L_THIRTY_DIR=../../../build/script/Provider

#----------------------- functions ---------------------#
L_PACKAGE_NAME=$1
L_PRODUCT_TYPE=`echo $2 | tr A-Z a-z`
L_PRODUCT=`echo $3 | tr A-Z a-z`

if [ "debug" == "$2" ];then
    G_BUILD_OPTION=debug
	export DEBUG=debug
fi

if [ "openssl-oldversion" == "$3" ];then
export openssl_version=openssl-1.0.2r
export curl_version=curl-7.64.1
else
export openssl_version=openssl-1.1.1d
export curl_version=curl-7.66.0
fi
export libxml2_version=libxml2-2.9.9
#export nghttp2_version=nghttp2-1.32.0
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

export BUILD_FOR_MACOS=true

# **************************************************************************** #
# Function Name: compileThirty
# Description: 
# Parameter:  $1 SLPAgent目录
# Return: non
# **************************************************************************** #
compileThirty()
{
    eval 'L_TMP_THIRTY_DIR=${'$1'}'
    if [ ! -d ${G_THIRTY_DIR} ];then
        echo "no such diretory ${L_TMP_THIRTY_DIR}."
        exit 1
    fi
    
    cd ${L_TMP_THIRTY_DIR}
    if [ "build_ci" != "$L_PRODUCT" ];then
	    bash build_logAPI_staic.sh
		ifFailExitAndPrint $? "make failed."
		
        bash build_pcre.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."
    
        bash build_openssl.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."

        bash build_iconv.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."  
	   	
        bash build_libxml2.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."

#        bash build_nghttp2.sh
#		ifFailExitAndPrint $? "make failed."

        bash build_curl.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."
	fi	     
    cd ${G_BUILD_DIR}
}

#----------编译third_party_groupware------------
#compileThirty L_THIRTY_DIR

#cd ${G_BUILD_DIR}

#----------编译libsecurec.so------------
pushd $G_SECUREC_PATH/src >/dev/null
make -f Makefile.Macos clean

make -f Makefile.Macos
popd >/dev/null
make clean
make

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

cp -f ${g_PATH}/include/* include
cp -f ${g_PATH}/lib/*.dylib lib
cp -f ./../../../platform/huaweisecurec/include/* include
cp -f ./../../../platform/huaweisecurec/lib/libsecurec.dylib lib
cp -af ./../../../platform/eSDK_LogAPI_V2.1.10/C/macos/libeSDKLogAPI.dylib lib
cp -af ./../../../build/script/Provider/build/macos/${curl_version}/lib/*.dylib lib
cp -af ./../../../build/script/Provider/build/macos/${libxml2_version}/lib/*.dylib lib
cp -af ./../../../build/script/Provider/build/macos/${openssl_version}/lib/*.dylib lib 
cp -af ./../../../build/script/Provider/build/macos/pcre-8.39/lib/*.dylib lib 
cp -f Makefile_obs demo/Makefile
cp -f OBS.ini lib
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/object_test.c" demo/object_test.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo.c" demo/demo.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.c" demo/demo_common.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.h" demo/demo_common.h
cp -f cert/client.pem demo/client.pem
cp -f cert/client.pem lib/client.pem
tar zcvf ${L_PACKAGE_NAME}.tgz demo include lib readme.txt

#cp -f ${g_PATH}/include/* include_static
#cp -f ${g_PATH}/lib/*.a lib_static
#cp -f ./../../../platform/huaweisecurec/include/* include_static
#cp -f ./../../../platform/huaweisecurec/src/libsecurec.a lib_static
#cp -f ./../../../platform/eSDK_LogAPI_V2.1.10/eSDKLogAPI/libeSDKLogAPI.a lib_static
#cp -f /usr/local/log4cpp/lib/*.a lib_static
#cp -f ./../../../build/script/Provider/build/linux/${curl_version}/static_package/lib/* lib_static
#cp -f ./../../../build/script/Provider/build/linux/${libxml2_version}/static_package/lib/* lib_static
#cp -f ./../../../build/script/Provider/build/linux/${openssl_version}/static_package/lib/* lib_static
#cp -f ./../../../build/script/Provider/build/linux/pcre-8.39/static_package/lib/* lib_static 
#cp -f ./../../../build/script/Provider/build/linux/iconv-1.15/static_package/lib/* lib_static 
#cp -f ./../../../build/script/Provider/build/linux/${nghttp2_version}/static_package/lib/* lib_static 
#cp -f Makefile_static demo_static/Makefile
#cp -f OBS.ini lib_static
#cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/object_test.c" demo_static/object_test.c
#cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo.c" demo_static/demo.c
#cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.c" demo_static/demo_common.c
#cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.h" demo_static/demo_common.h
#cp -f cert/client.pem demo_static/client.pem
#cp -f cert/client.pem lib_static/client.pem
#tar zcvf ${L_PACKAGE_NAME}_STATIC.tgz demo_static include_static lib_static readme.txt

rm -rf {demo,include,lib,"Log Collection Statement.txt"}
rm -rf {demo_static,include_static,lib_static,"Log Collection Statement.txt"}
