#!/bin/bash
#Usage: build.sh packageName release|debug
#packageName Ϊ""ʱ �����

# ����bin
# ����include
# ����lib
#----------------------- variables --------------------#
#��ǰ�ű�����·��
G_CWD=`dirname $0`
pushd $G_CWD >/dev/null
G_CWD=`pwd`
popd >/dev/null

G_FILE_NAME=$0
G_BUILD_OPTION=release
G_BUILD_DIR=${G_CWD}
g_PATH=build

G_SECUREC_PATH=$G_CWD/../../../platform/libboundscheck

#THIRTY_DIRĿ¼
G_THIRTY_DIR=$G_CWD/../../../build/script/Provider
L_THIRTY_DIR=../../../build/script/Provider
G_PLATFORM=$G_CWD/../../../platform/
#----------------------- functions ---------------------#
L_PACKAGE_NAME=$1
L_PRODUCT_TYPE=`echo $2 | tr A-Z a-z`
L_PRODUCT=`echo $3 | tr A-Z a-z`

CMAKE_BUILD_TYPE=Release
if [ "debug" == "$2" ];then
    G_BUILD_OPTION=debug
    CMAKE_BUILD_TYPE=Debug
	export DEBUG=debug
fi

#if [ "openssl-oldversion" == "$3" ];then
#export openssl_version=openssl-1.0.2r
#export curl_version=curl-7.64.1
#export pcre_version=pcre-8.39
#export iconv_version=iconv-1.15
#export libxml2_version=libxml2-2.9.9
#else
export openssl_version=openssl-1.1.1w
export curl_version=curl-8.11.1
export pcre_version=pcre-8.45
export iconv_version=iconv-1.15
export libxml2_version=libxml2-2.9.9
export cjson_version=cjson-1.7.18
#fi
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

# **************************************************************************** #
# Function Name: compileThirty
# Description: 
# Parameter:  $1 SLPAgentĿ¼
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

#----------����third_party_groupware------------
#compileThirty L_THIRTY_DIR

#cd ${G_BUILD_DIR}

#----------����libsecurec.so------------
#pushd $G_SECUREC_PATH/src >/dev/null
#make clean

# make
popd >/dev/null
make clean
rm -rf cmake-build
mkdir cmake-build
cd cmake-build
mkdir cmake
cd cmake
cmake $G_CWD/../../../ -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DSPDLOG_VERSION=${SPDLOG_VERSION}
make 

if [ 0 -ne $? ];then
    echo 'make failed in build.sh'
	exit 1
fi

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
cp -f cmake-build/cmake/lib/*.so lib
cp -f ./../../../platform/libboundscheck/include/* include
cp -f ./../../../platform/libboundscheck/lib/linux/*.so lib
cp -af ./../../../platform/eSDK_LogAPI_V2.1.10/C/linux_64/libeSDKLogAPI.so lib
cp -af ./../../../build/script/Provider/build/linux/${curl_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/linux/${libxml2_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/linux/${openssl_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/linux/${pcre_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/linux/${iconv_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/linux/${cjson_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/linux/${SPDLOG_VERSION}/lib/* lib
cp -f ./../../../build/script/Provider/build/linux/${cjson_version}/include/cJSON.h include
#cp -f ./../../../build/script/Provider/build/linux/${nghttp2_version}/lib/* lib 
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
#cp -f ./../../../platform/libboundscheck/include/* include_static
#cp -f ./../../../platform/libboundscheck/src/*.a lib_static
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
