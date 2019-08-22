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

export openssl_version=openssl-1.0.2r
export curl_version=curl-7.64.1
export libxml2_version=libxml2-2.9.9
export nghttp2_version=nghttp2-1.32.0
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
#alias gcc=aarch64-linux-gnu-gcc
export BUILD_FOR_ARM=true
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
    echo "thirdparty =${L_TMP_THIRTY_DIR}"
    cd ${L_TMP_THIRTY_DIR}
    
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
		
		bash build_nghttp2.sh
		ifFailExitAndPrint $? "make failed."
		
        bash build_curl.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."
        
		bash build_logAPI.sh
		ifFailExitAndPrint $? "make failed."
		     
    cd ${G_BUILD_DIR}
}

#----------编译third_party_groupware------------
#compileThirty L_THIRTY_DIR

#cd ${G_BUILD_DIR}

#----------编译libsecurec.so------------
#echo "$G_SECUREC_PATH/src"
pushd $G_SECUREC_PATH/src >/dev/null
make clean

make -f Makefile.aarch64
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

if [ "$G_BUILD_OPTION" == "debug" ];then
g_PATH=build-debug
fi

cp -f ${g_PATH}/include/* include
cp -f ${g_PATH}/lib/*.so lib
cp -f ./../../../platform/huaweisecurec/include/* include
cp -f ./../../../platform/huaweisecurec/lib/libsecurec.so lib
cp -f ./../../../platform/eSDK_LogAPI_V2.1.10/C/aarch64/libeSDKLogAPI.so lib
cp -f ./../../../platform/eSDK_LogAPI_V2.1.10/log4cpp/lib/* lib 
cp -f ./../../../build/script/Provider/build/arm/${curl_version}/lib/* lib
cp -f ./../../../build/script/Provider/build/arm/${libxml2_version}/lib/* lib
cp -f ./../../../build/script/Provider/build/arm/${openssl_version}/lib/* lib 
cp -f ./../../../build/script/Provider/build/arm/pcre-8.39/lib/* lib 
cp -f ./../../../build/script/Provider/build/arm/iconv-1.15/lib/* lib 
cp -f ./../../../build/script/Provider/build/arm/${nghttp2_version}/lib/* lib 
echo "BUILD_FOR_ARM=true" >  demo/Makefile
cat Makefile_obs >> demo/Makefile
cp -f OBS.ini lib
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/object_test.c" demo/object_test.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo.c" demo/demo.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.c" demo/demo_common.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.h" demo/demo_common.h
cp -f cert/server.jks demo/server.jks
cp -f cert/client.crt demo/client.crt
tar zcvf ${L_PACKAGE_NAME}.tgz demo include lib readme.txt

rm -rf {demo,include,lib,"Log Collection Statement.txt"}
