#!/bin/sh

# Requirement: the environment where you want to build mongodb driver ,must install jdk-1.8.x
#              and gradle.

# include build_lib.sh
SCRIPT_PATH="$(cd $(dirname $0);pwd)"
. ${SCRIPT_PATH}/build_lib.sh
CUR_DIR=$(getCurPath)
INDEX_WORKSPACE=$(echo ${CUR_DIR%\/*})
DRIVE_PATH_IN_CI=/home/workspace/dfv_index_layer_uniautos_test/dfvtest/lib/index/
current_file=$0
log "INFO: excute ${current_file}."
export PATH=$PATH:/usr/local/gradle-3.5/bin
log "INFO: PATH = ${PATH}"

# define variables
SRC_DIR=${INDEX_WORKSPACE}/src
MONGO_DRIVER_PATH=${SRC_DIR}/mongo-driver
TARGET_JAR_PATH=${MONGO_DRIVER_PATH}/mongo-java-driver/build/libs

# clean target jar path
rm -rf ${TARGET_JAR_PATH}/*.jar

# compile dfv-index-driver.jar
cd ${MONGO_DRIVER_PATH}
gradle jar --info
retcode=$?
if [ ${retcode} -ne 0 ];then 
    log "ERROR: build dfv-index-driver failed."
    exit ${COMPILE_ERROR}
else
    if [ -d ${DRIVE_PATH_IN_CI} ];then
        cp -f ${TARGET_JAR_PATH}/*.jar ${DRIVE_PATH_IN_CI}
        log "INFO: copy ${TARGET_JAR_PATH}/*.jar to ${DRIVE_PATH_IN_CI}"
    fi
fi

log "INFO: finished to execute $0 ."
exit 0
