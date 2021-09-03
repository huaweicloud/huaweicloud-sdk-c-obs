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

G_SECUREC_PATH=$G_CWD/../../../platform/huaweisecurec

#THIRTY_DIRĿ¼
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
alias gcc=aarch64-linux-android-gcc
export BUILD_FOR_NDK_AARCH64=true
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
		
        bash build_curl.sh
        ifFailExitAndPrint $? "make failed."
#       read -p "Press any key to continue."
        
		bash build_logAPI.sh
		ifFailExitAndPrint $? "make failed."
		     
    cd ${G_BUILD_DIR}
}

getThirdparty()
{
    CUR_DIR=$(cd $(dirname $0);pwd)
    THIRDPARTY_DIR=$CUR_DIR/../../../build/script/Provider/build/ndk-aarch64
    if [ ! -d $THIRDPARTY_DIR ];then
        mkdir -p $THIRDPARTY_DIR
    fi
    #openssl
    bm --action download --name openssl --version 1.1.1.d --release 106415 --token ${bm_user_token} --output ${THIRDPARTY_DIR}
    tar xzf ${THIRDPARTY_DIR}/openssl-*.tar.gz -C ${THIRDPARTY_DIR}

    #libcurl
    bm --action download --name libcurl --version 7.66.0 --release 106412 --token ${bm_user_token} --output ${THIRDPARTY_DIR}
    tar xzf ${THIRDPARTY_DIR}/libcurl-*.tar.gz -C ${THIRDPARTY_DIR}

    #libxml2
    bm --action download --name libxml2 --version 2.9.9 --release 106414 --token ${bm_user_token} --output ${THIRDPARTY_DIR}
    tar xzf ${THIRDPARTY_DIR}/libxml2-*.tar.gz -C ${THIRDPARTY_DIR}

    #libiconv
    bm --action download --name libiconv --version 1.15 --release 106413 --token ${bm_user_token} --output ${THIRDPARTY_DIR}
    tar xzf ${THIRDPARTY_DIR}/libiconv-*.tar.gz -C ${THIRDPARTY_DIR}

    #pcre
    bm --action download --name libpcre --version 8.39 --release 106419 --token ${bm_user_token} --output ${THIRDPARTY_DIR}
    tar xzf ${THIRDPARTY_DIR}/pcre-*.tar.gz -C ${THIRDPARTY_DIR}
}

#----------����third_party_groupware------------
#compileThirty L_THIRTY_DIR

#cd ${G_BUILD_DIR}

getThirdparty

#----------����libsecurec.so------------
bash $L_THIRTY_DIR/build_logAPI.sh

pushd $G_SECUREC_PATH/src >/dev/null
make clean

make -f Makefile.ndk-aarch64
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

bm --action download --name eSDK_LogAPI --version "2.1.10" --release latest --token ${bm_user_token} --output ${G_PLATFORM}
bm --action download --name obs-sdk-c-third-lib --version "1.0.0" --release latest --token ${bm_user_token} --output ${G_THIRTY_DIR}

tar xzf ${G_PLATFORM}/eSDK_LogAPI_*.tar -C ${G_PLATFORM}
tar xzf ${G_THIRTY_DIR}/esdk-obs-c-third-lib.tar -C ${G_THIRTY_DIR}

rm ${G_PLATFORM}/eSDK_LogAPI_*.tar ${G_THIRTY_DIR}/esdk-obs-c-third-lib.tar

cp -f ${g_PATH}/include/* include
cp -f ${g_PATH}/lib/*.so lib
cp -f ./../../../platform/huaweisecurec/include/* include
cp -f ./../../../platform/huaweisecurec/lib/libsecurec.so lib
cp -f ./../../../platform/eSDK_LogAPI_V2.1.10/C/ndk-aarch64/libeSDKLogAPI.so lib
#cp -af ./../../../platform/eSDK_LogAPI_V2.1.10/C/ndk-aarch64/liblog4cpp* lib 
cp -af ./../../../build/script/Provider/build/ndk-aarch64/${curl_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/ndk-aarch64/${libxml2_version}/lib/* lib
cp -af ./../../../build/script/Provider/build/ndk-aarch64/${openssl_version}/lib/* lib 
cp -af ./../../../build/script/Provider/build/ndk-aarch64/pcre-8.39/lib/* lib 
cp -af ./../../../build/script/Provider/build/ndk-aarch64/iconv-1.15/lib/* lib 
echo "BUILD_FOR_NDK_AARCH64=true" >  demo/Makefile
cat Makefile_obs >> demo/Makefile
cp -f OBS.ini lib
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/object_test.c" demo/object_test.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo.c" demo/demo.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.c" demo/demo_common.c
cp -f "./../../../source/eSDK_OBS_API/eSDK_OBS_API_C++_Demo/demo_common.h" demo/demo_common.h
cp -f cert/server.jks demo/server.jks
cp -f cert/client.crt demo/client.crt
aarch64-linux-android-strip -s ./lib/*.so*
tar zcvf ${L_PACKAGE_NAME}.tgz demo include lib readme.txt

rm -rf {demo,include,lib,"Log Collection Statement.txt"}
