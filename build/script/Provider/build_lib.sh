#!/bin/sh

# module and version info
ROCKSDB_VERSION=5.1.2
MONGODB_VERSION=3.4.0
MONGO_ROCKS_VERSION=3.4.0
LZ4_VERSION=1.7.5
FTDS_PACKAGE_NAME=FTDS-client-SDK-release
FTDS_DIR_NAME=FTDS-client-SDK
MAVEN_BASE_URL="http://szxy1.artifactory.cd-cloud-artifact.tools.huawei.com/artifactory/product-dfv/"
FTDS_BASE_URL="${MAVEN_BASE_URL}com/huawei/dfv/OAM_master/FTDS-client-SDK-release"
MICRO_PACKAGE_NAME=DFV_indexlayer
ZKCLI_VERSION=1.9.T1
ZKCLI_NAME=zookeeper_client_lib

# define error code
COMPILE_ERROR=1
PACKAGE_ERROR=2
UPLOAD_DRIVER_ERROR=3

# Build settings
OPT_SWITCH=on
NO_CLEAN=false

# get workspace folder
function getCurPath()
{
    cd "$(dirname "$0")"
    echo "$(pwd)"
    cd - > /dev/null 2>&1
    return 0
}

# get log info
SHELL_LOG_PATH=/var/log/dfv/install
if [ ! -d ${SHELL_LOG_PATH} ];then
    mkdir -p ${SHELL_LOG_PATH}
fi
SHELL_LOG=${SHELL_LOG_PATH}/index.log
if [ ! -e ${SHELL_LOG} ];then
    touch ${SHELL_LOG}
fi
function log()
{
    echo "[`date "+%Y-%m-%d %H:%M:%S"`] $*" >> ${SHELL_LOG}
    echo "[`date "+%Y-%m-%d %H:%M:%S"`] $*"
}

# set ccache config 
function set_ccache()
{
    #add path for ccache
    source /etc/profile
    export PATH=/root/bin:$PATH
    CACHE_BASE=""
    if [[ $1 == "rocksdb" ]]; then
        CACHE_BASE=/home/CCACHE_FOR_ROCKSDB
    else
        CACHE_BASE=/home/CCACHE_FOR_MONGODB
    fi
    # $0 is build_mongdb_mock.sh or build_mongodb_real.sh
    exe_file=$(echo $0 | awk -F '/' '{print $NF}')
    if [[ ${exe_file} =~ "mock" ]]; then
        CACHE_BASE="${CACHE_BASE}_MOCK"
    fi
    if [ ! -d ${CACHE_BASE} ]; then
        mkdir -p $CACHE_BASE
    fi
    chmod -R 777 $CACHE_BASE
    log "INFO: CCACHE_DIR=$CACHE_BASE"
    export USE_CCACHE=1
    export CCACHE_DIR=$CACHE_BASE
    export CCACHE_UMASK=002
    /usr/local/bin/ccache -M 4G
}

# caculate time
starttime=""
endtime=""
duration=""
proc_name=""
function print_starttime()
{
    starttime=`date +'%Y-%m-%d %H:%M:%S'`
    log "INFO: $proc_name  starttime = $starttime."
}

function print_endtime()
{
    endtime=`date +'%Y-%m-%d %H:%M:%S'`
    start_seconds=$(date --date="$starttime" +%s);
    end_seconds=$(date --date="$endtime" +%s);
    duration=$((end_seconds-start_seconds))
    log "INFO: $proc_name  endtime   = $endtime. duration = $duration(S)."
}

# get component port by name
function get_component_port()
{
    COMPONENT_NAME=$1
    COMPONENT_PORT=""
    if [[ ${COMPONENT_NAME} == "config" ]];then
        COMPONENT_PORT=27015
    elif [[ ${COMPONENT_NAME} == "shard" ]];then
        COMPONENT_PORT=27021
    elif [[ ${COMPONENT_NAME} == "client" ]];then
        COMPONENT_PORT=27010
    else
        log "ERROR: ${COMPONENT_NAME} is invalid name."
        exit ${PACKAGE_ERROR}
    fi
    echo ${COMPONENT_PORT}
}

#########################################################
# COMMON BUILD FUNCTION
#########################################################

# compile LZ4
function compile_lz4()
{
    log "INFO: start compile LZ4."
    cd ${WGET_PATH}
    cp -f ${THIRD_PATH}/${LZ4_NAME}.tar.gz .
    if [ -f ${LZ4_NAME}.tar.gz ];then
        rm -rf ./${LZ4_NAME}
        log "INFO: rm -rf ${WGET_PATH}/${LZ4_NAME}"
        tar -xzvf ${LZ4_NAME}.tar.gz
        cd ${LZ4_NAME}
        make -j ${CPU_CORES}
        make install
        retcode=$?
        if [ ! -e $WGET_PATH/${LZ4_NAME}/lib/liblz4.so.${LZ4_VERSION} ] || [ ${retcode} -ne 0 ];then
            log "ERROR: compile LZ4 failed."
            exit ${COMPILE_ERROR}
        fi
        cp -rf ${WGET_PATH}/${LZ4_NAME}/lib/liblz4.so* ${PCLIENTX_DIR}/
        cd -
        rm ${LZ4_NAME}.tar.gz
        log "INFO: rm ${LZ4_NAME}.tar.gz"
        rm -rf ./${LZ4_NAME}
        log "INFO: rm -rf ${WGET_PATH}/${LZ4_NAME}"
    else
        log "ERROR: copy lz4-${LZ4_VERSION}.tar.gz failed."
        exit ${COMPILE_ERROR}
    fi
    log "INFO: succeed compile LZ4."
}

# compile plog_client
function compile_plog_client()
{
    log "INFO: start compile plog_client of $1."
    rm -f /usr/lib64/libplogclientx.so
    cd ${PLOGCLIENT_PATH}
    if [[ $1 == "mock" ]]; then
        sh mock-make.sh mock_shared_lib
    else
        export LIBRARY_PATH=${WGET_PATH}/${PLAYERSDK_NAME}/c/lib:${LIBRARY_PATH}
        log "INFO: LIBRARY_PATH is ${LIBRARY_PATH}"
        sh mock-make.sh player_shared_lib
    fi
    retcode=$?
    if [ ! -e libplogclientx.so ] || [ ${retcode} -ne 0 ];then
        log "ERROR: compile plog_client failed."
        exit ${COMPILE_ERROR}
    fi
    cp -f libplogclientx.so ${PCLIENTX_DIR}
    log "INFO: succeed compile plog_client."
}

# compile indexbase
function compile_index_base()
{
    if [ "$NO_CLEAN" = true ]; then
        return 0
    fi  
    # And copy ftds.h *.so
    cd ${WGET_PATH}
    FTDS_VER=""
    if [[ $1 == "mock" ]]; then
        FTDS_VER="1.0.15"
        log "INFO: FTDS [1.0.15] used for mock."
    else
        wget "${FTDS_BASE_URL}/maven-metadata.xml"
        FTDS_VER=`awk -v RS="</*latest>" 'NR==2{print}' maven-metadata.xml`
        log "INFO: get ftds version [${FTDS_VER}] from maven-metadata.xml."
    fi

    wget "${FTDS_BASE_URL}/${FTDS_VER}/${FTDS_PACKAGE_NAME}-${FTDS_VER}.tar.gz"
    if [ -f ${FTDS_PACKAGE_NAME}-${FTDS_VER}.tar.gz ];then
        rm -rf ./${FTDS_DIR_NAME}
        log "INFO: rm -rf ${WGET_PATH}/${FTDS_DIR_NAME}"
        tar -xvzf  ${FTDS_PACKAGE_NAME}-${FTDS_VER}.tar.gz
        
        rm $MONGO_JAVA_DRIVER/driver-core/libs/dfv-ftds-*.jar
        cp -rf ${WGET_PATH}/${FTDS_DIR_NAME}/client_java/dfv-ftds-${FTDS_VER}.jar $MONGO_JAVA_DRIVER/driver-core/libs/
        sed -i "/dfv-ftds/d"  $MONGO_JAVA_DRIVER/driver-core/build.gradle
        sed -i "/dependencies {/a\    compile files('libs/dfv-ftds-${FTDS_VER}.jar')"  $MONGO_JAVA_DRIVER/driver-core/build.gradle
        rm $MONGO_JAVA_DRIVER/mongo-java-driver/libs/dfv-ftds-*.jar
        cp -rf ${WGET_PATH}/${FTDS_DIR_NAME}/client_java/dfv-ftds-${FTDS_VER}.jar $MONGO_JAVA_DRIVER/mongo-java-driver/libs/
        sed -i "/dfv-ftds/d"  $MONGO_JAVA_DRIVER/mongo-java-driver/build.gradle
        sed -i "/dependencies {/a\    compile files('libs/dfv-ftds-${FTDS_VER}.jar')"  $MONGO_JAVA_DRIVER/mongo-java-driver/build.gradle

        cp -rf ${WGET_PATH}/${FTDS_DIR_NAME}/client_c/ftds.h  ${INDEX_WORKSPACE}/public_inc/
        cp -rf ${WGET_PATH}/${FTDS_DIR_NAME}/client_c/*.so* ${PCLIENTX_DIR}/
        cp -rf ${WGET_PATH}/${FTDS_DIR_NAME}/client_c/ftds_stat ${FTDS_PATH}/tools/
        rm ${FTDS_PACKAGE_NAME}-${FTDS_VER}.tar.gz
        log "INFO: rm ${FTDS_PACKAGE_NAME}-${FTDS_VER}.tar.gz"
        rm -rf ./${FTDS_DIR_NAME}
        log "INFO: rm -rf ${WGET_PATH}/${FTDS_DIR_NAME}"
        rm -rf maven-metadata.xml
    elif [ -d ${INDEX_WORKSPACE}/third/${FTDS_DIR_NAME} ];then
        cp -rf ${INDEX_WORKSPACE}/third/${FTDS_DIR_NAME}/client_c/ftds.h  ${INDEX_WORKSPACE}/public_inc/
        cp -rf ${INDEX_WORKSPACE}/third/${FTDS_DIR_NAME}/client_c/*.so* ${PCLIENTX_DIR}/
        cp -rf ${INDEX_WORKSPACE}/third/${FTDS_DIR_NAME}/client_c/ftds_stat ${FTDS_PATH}/tools/
        log "INFO: copy ftds from local"
    else    
        log "ERROR: wget ${FTDS_PACKAGE_NAME} failed."
        exit ${COMPILE_ERROR}
    fi

    # get zkcli
    cd ${WGET_PATH}
    log "INFO: get ${ZKCLI_NAME}."
    mvn dependency:get  -DgroupId=com.huawei.dfv.Infrastructure -DartifactId=${ZKCLI_NAME} -Dversion=${ZKCLI_VERSION}-SNAPSHOT -Dpackaging=tar.gz  -DremoteRepositories=CloudArtifact-snapshots::default::${MAVEN_BASE_URL} -Ddest=${ZKCLI_NAME}-${ZKCLI_VERSION}.tar.gz
    if [ -f ${ZKCLI_NAME}-${ZKCLI_VERSION}.tar.gz ];then
        rm -rf ${WGET_PATH}/${ZKCLI_NAME}-${ZKCLI_VERSION}
        tar -xvzf ${ZKCLI_NAME}-${ZKCLI_VERSION}.tar.gz
        cp -rf ${ZKCLI_NAME}_${ZKCLI_VERSION}/clib/*  ${PCLIENTX_DIR}/
        log "INFO: copy ${ZKCLI_NAME}-${ZKCLI_VERSION}/clib/"
        if [[ -n "${AZ_SWITCH}" ]]; then
            mkdir -p ${ZK_CLI_JAR}
            cp -rf ${ZKCLI_NAME}_${ZKCLI_VERSION}/jlib/* ${ZK_CLI_JAR}
            log "INFO: copy ${ZKCLI_NAME}_${ZKCLI_VERSION}/jlib/"
            # get java
            cd ${SCRIPT_PATH}
            wget "http://szxy1.artifactory.cd-cloud-artifact.tools.huawei.com/artifactory/product-dfv/com/huawei/dfv/OpenJDK/maven-metadata.xml"
            JDK_VER=`awk -v RS="</*latest>" 'NR==2{print}' maven-metadata.xml`
            wget "http://szxy1.artifactory.cd-cloud-artifact.tools.huawei.com/artifactory/product-dfv/com/huawei/dfv/OpenJDK/${JDK_VER}/OpenJDK-${JDK_VER}.tar.bz2"
            tar jxf OpenJDK-${JDK_VER}.tar.bz2
            mkdir -p ${JDK_DIR}
            cp -rf j2sdk-image/jre ${JDK_DIR}/
        fi
    else
        log "ERROR: wget ${ZKCLI_NAME} failed."
        exit ${COMPILE_ERROR}
    fi
    rm -rf ${WGET_PATH}/${ZKCLI_NAME}-${ZKCLI_VERSION}*
    rm -rf maven-metadata.xml
    rm -rf OpenJDK-${JDK_VER}*

    # compile indexpubinc
    log "INFO: start compile index public include."
    export LIBRARY_PATH=${PCLIENTX_DIR}:${LIBRARY_PATH}
    cd $PUBLIC_INC_PATH
    rm -f /usr/lib64/libindexpubinc.so
    sh public-make.sh
    retcode=$?
    if [ ! -e $PUBLIC_INC_PATH/libindexpubinc.so ] || [ ${retcode} -ne 0 ];then
        log "ERROR: compile index public include failed."
        exit ${COMPILE_ERROR}
    fi
    cp -f libindexpubinc.so ${PCLIENTX_DIR}/
    log "INFO: succeed compile index public include."

    # compile indexbase
    log "INFO: start compile indexbase."
    export LIBRARY_PATH=${PCLIENTX_DIR}:${LIBRARY_PATH}
    cd ${SRC_COMMON}/base
    rm -f /usr/lib64/libindexbase.so
    sh base-make.sh
    retcode=$?
    if [ ! -e ${SRC_COMMON}/base/libindexbase.so ] || [ ${retcode} -ne 0 ];then
        log "ERROR: compile indexbase failed."
        exit ${COMPILE_ERROR}
    fi
    cp -f libindexbase.so ${PCLIENTX_DIR}/
    log "INFO: succeed compile indexbase."
}

# compile rocksdb

ROCKSDB_COMPILE_PREPARED=no
function prepare_compile_rocksdb()
{
    if [ "$ROCKSDB_COMPILE_PREPARED" = yes ]; then
        return
    fi

    log "INFO: start compile $1."
    export LIBRARY_PATH=${PCLIENTX_DIR}:${LIBRARY_PATH}
    proc_name="build rocksdb"
    if [ "${CCACHE_SWITCH}" == "" ];then
        set_ccache "rocksdb"
    fi
    print_starttime
    cd ${ROCKSDB_PATH}

    if [ ! "$NO_CLEAN" = true ]; then
        make clean
        retcode=$?
        if [ -e librocksdb.so.${ROCKSDB_VERSION} ] || [ ${retcode} -ne 0 ];then
            log "ERROR: clean rocksdb failed."
            exit ${COMPILE_ERROR}
        fi
    fi
    
    if [ "$OPT_SWITCH" = off ]; then
        debug_enabled=true
    else
        debug_enabled=false
    fi
    
    ROCKSDB_COMPILE_PREPARED=yes
}

function compile_rocksdb_only()
{
    prepare_compile_rocksdb "rocksdb"
    make shared_lib -j ${CPU_CORES} EXTRA_CXXFLAGS="-I$SRC_COMMON" DEBUG=$debug_enabled
    retcode=$?
    if [ ! -e librocksdb.so.${ROCKSDB_VERSION} ] || [ ${retcode} -ne 0 ];then
        log "ERROR: compile rocksdb failed."
        exit ${COMPILE_ERROR}
    fi
    
    INSTALL_PATH=/usr make install
    ldconfig
    print_endtime
    log "INFO: succeed compile & install rocksdb."
}

function compile_db_bench()
{
    prepare_compile_rocksdb "db_bench"
    make db_bench_bin -j ${CPU_CORES} DEBUG=$debug_enabled
    if [ ! -e db_bench ]; then
        log "ERROR: compile db_bench failed."
        exit ${COMPILE_ERROR}
    fi
}

function compile_backup_check_tool()
{
    log "INFO: start compile backup_check tool."
    print_starttime
    export LIBRARY_PATH=${PCLIENTX_DIR}:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${PCLIENTX_DIR}:${LIBRARY_PATH}
    cd ${BACKUP_CHECK_TOOL_PATH}
    make clean
    log "$1"
    rm -f backup_check
    if [[ $1 == "mock" ]]; then
        make backup_check_mock
    else
        make backup_check
    fi

    if [ ! -e backup_check ]; then
        log "ERROR: compile backup_check failed."
        exit ${COMPILE_ERROR}
    fi
    print_endtime
}


function compile_db_recovery_tool()
{
    prepare_compile_rocksdb "db_recovery"
    make db_recovery_tool -j ${CPU_CORES} DEBUG=$debug_enabled
    if [ ! -e db_recovery ]; then
        log "ERROR: compile db_recovery failed."
        exit ${COMPILE_ERROR}
    fi
}

function compile_db_rescue_tool()
{
    prepare_compile_rocksdb "db_rescue_tool"
    make db_rescue_tool -j ${CPU_CORES} EXTRA_CXXFLAGS="-I$SRC_COMMON" DEBUG=$debug_enabled
    if [ ! -e db_rescue ]; then
        log "ERROR: compile db_rescue failed."
        exit ${COMPILE_ERROR}
    fi
}

function compile_rocksdb()
{
    compile_rocksdb_only
    
    # build db_bench if mock
    if [[ $1 == "mock" ]]; then
        compile_db_bench
    fi
     
    # build db_recovery_tool
    compile_db_recovery_tool

    # build db_rescue_tool
    compile_db_rescue_tool
    
    # build backup_check tool
    compile_backup_check_tool $1

}

# compile tcmalloc
function compile_tcmalloc()
{
    log "INFO: start compile tcmalloc."
    # debug version
    cd ${TCMALLOC_PATH}
    sed -i '/#define __DEBUG_TC_MALLOC_HEADER__/c\#define __DEBUG_TC_MALLOC_HEADER__ 1' ${TCMALLOC_PATH}/src/tcmalloc.cc
    make clean
    ./configure
    make -j ${CPU_CORES}
    if [ ! -e "${TCMALLOC_PATH}/.libs/libtcmalloc.so.4.3.0" ]; then
        log "ERROR: complie tcmalloc failed."
        exit ${COMPILE_ERROR}
    fi
    cp -f ${TCMALLOC_PATH}/.libs/libtcmalloc.so.4.3.0 /${PCLIENTX_DIR}/libtcmalloc_debug.so.4.3.0
    log "INFO: succeed compile debug tcmalloc."

    # no-debug version [default version]
    cd ${TCMALLOC_PATH}
    sed -i '/#define __DEBUG_TC_MALLOC_HEADER__/c\#define __DEBUG_TC_MALLOC_HEADER__ 0' ${TCMALLOC_PATH}/src/tcmalloc.cc
    make clean
    ./configure
    make -j ${CPU_CORES}
    if [ ! -e "${TCMALLOC_PATH}/.libs/libtcmalloc.so.4.3.0" ]; then
        log "ERROR: complie tcmalloc failed."
        exit ${COMPILE_ERROR}
    fi
    cp -f ${TCMALLOC_PATH}/.libs/libtcmalloc.so.4.3.0 /${PCLIENTX_DIR}/libtcmalloc.so.4.3.0
    cp -f ${TCMALLOC_PATH}/.libs/libtcmalloc_minimal.a ${TCMALLOC_PATH}/
    log "INFO: succeed compile tcmalloc."

    cd ${PCLIENTX_DIR}
    ln -sf libtcmalloc.so.4.3.0 libtcmalloc.so
    ln -sf libtcmalloc.so.4.3.0 libtcmalloc.so.4
}

function compile_mongodb_only()
{
    log "INFO: start compile mongodb."
    proc_name="build mongodb"
    if [ "${CCACHE_SWITCH}" == "" ];then
        set_ccache "mongodb"
    fi
    print_starttime
    cp -rf ${TCMALLOC_PATH}/src/gperftools /usr/local/include/
    cp -rf ${TCMALLOC_PATH}/src/google /usr/local/include/
    
    if [ ! -d "${MONGO_PATH}/build/opt/third_party" ]; then
        mkdir -p ${MONGO_PATH}/build/opt/third_party
    fi
    touch ${MONGO_PATH}/build/opt/third_party/libshim_allocator.a
    export LIBRARY_PATH=${PCLIENTX_DIR}:${TCMALLOC_PATH}/.libs:${LIBRARY_PATH}

    cd ${MONGO_PATH}
cat <<EOF > version.json
{
    "version" : "3.4.0-release"
}
EOF
    mkdir -p src/mongo/db/modules/
    ln -sf ${MONGOROCKS_PATH} src/mongo/db/modules/rocks

    if [ ! "$NO_CLEAN" = true ]; then
        scons $1 -c
        retcode=$?
        if [[ $1 == "core" ]];then
            if [ -e mongo ] || [ -e mongod ] || [ -e mongos ] || [ ${retcode} -ne 0 ];then
                log "ERROR: clean mongodb failed."
                exit ${COMPILE_ERROR}
            fi
        else
            if [ -e $1 ] || [ ${retcode} -ne 0 ];then
                log "ERROR: clean mongodb $1 failed."
                exit ${COMPILE_ERROR}
            fi
        fi
    fi

    CPPDEFINES=""
    if [[ -n "${AZ_SWITCH}" ]]; then
        CPPDEFINES="BUILD_AZ"
    fi
    if [[ $2 == "mock" ]]; then
        CPPDEFINES="BUILD_MOCK ${CPPDEFINES}"
    fi
    log "INFO: mongodb CPPDEFINES ${CPPDEFINES}."
    if [[ -n "${CPPDEFINES}" ]]; then
        scons $1 -j ${CPU_CORES} --ssl -Q VERBOSE=1 CPPPATH="$SRC_COMMON $ROCKSDB_PATH/include" CPPDEFINES="${CPPDEFINES}" --opt=$OPT_SWITCH
    else
        scons $1 -j ${CPU_CORES} --ssl -Q VERBOSE=1 CPPPATH="$SRC_COMMON $ROCKSDB_PATH/include" --opt=$OPT_SWITCH
    fi

    retcode=$?
    if [[ $1 == "core" ]];then
        if [ ! -e mongo ] || [ ! -e mongod ] || [ ! -e mongos ] || [ ${retcode} -ne 0 ];then
            log "ERROR: compile mongodb failed."
            exit ${COMPILE_ERROR}
        fi
    else
        if [ ! -e $1 ] || [ ${retcode} -ne 0 ];then
            log "ERROR: compile mongodb $1 failed."
            exit ${COMPILE_ERROR}
        fi
    fi

    print_endtime
    log "INFO: succeed compile mongodb."
}

# compile mongodb (historic function)
function compile_mongodb()
{ 
    compile_tcmalloc
    compile_mongodb_only "$@"
}

# compile compmonitor
function compile_compmonitor()
{
    log "INFO: start compile compmonitor."
    cd ${MONITOR_PATH}
    if [ ! "$NO_CLEAN" = true ]; then
        sh clean_monitor.sh
        retcode=$?
        if [ -e compmonitor ] || [ ${retcode} -ne 0 ];then
            log "ERROR: clean compmonitor failed."
            exit ${COMPILE_ERROR}
        fi
    fi
    sh build_monitor.sh "${AZ_SWITCH}"
    retcode=$?
    if [ ! -e compmonitor ] || [ ${retcode} -ne 0 ];then
        log "ERROR: compile compmonitor failed."
        exit ${COMPILE_ERROR}
    fi
    log "INFO: succeed compile compmonitor."
}
