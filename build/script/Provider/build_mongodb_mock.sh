#!/bin/sh
#
#    Script to build "mock" build (PLog saves to local disk)
#    ./build_mongodb_mock.sh [goals] [--no_clean] [--no_opt]
#

# Requirement: the environment where you want to deploy mongodb must install
#            some basic packages, such as 'Development Tools'

BUILD_START=$(date +%s)
# include build_lib.sh
SCRIPT_PATH="$(cd $(dirname $0);pwd)"
. ${SCRIPT_PATH}/build_lib.sh

current_file=$0
log "INFO: excute ${current_file}."

source /etc/profile
user=$(id -un)
log "INFO: Current user is ${user}."
envinfo=$(env)
log "INFO: env is \"${envinfo}\"."

CUR_DIR=$(getCurPath)
INDEX_WORKSPACE=$(echo ${CUR_DIR%\/*})
BUILD_DIR=${INDEX_WORKSPACE}/build
WGET_PATH=${BUILD_DIR}
RELEASE_DIR=${BUILD_DIR}/release
SRC_DIR=${INDEX_WORKSPACE}/src
MONITOR_PATH=${SRC_DIR}/component-monitor
SRC_COMMON=${SRC_DIR}/common
ROCKSDB_PATH=${SRC_DIR}/rocksdb-${ROCKSDB_VERSION}
MONGOROCKS_PATH=${SRC_DIR}/mongo-rocks-r${MONGO_ROCKS_VERSION}
MONGO_PATH=${SRC_DIR}/mongo-r${MONGODB_VERSION}
PLOGCLIENT_PATH=${SRC_COMMON}/plogclient_stub
CPU_CORES=`cat /proc/cpuinfo | grep processor | wc -l`
PCLIENTX_DIR=${BUILD_DIR}/index_public/repo/lib
FTDS_PATH=${SRC_DIR}/common/base
FTDS_VER=""
THIRD_PATH=${INDEX_WORKSPACE}/third
LZ4_NAME=lz4-${LZ4_VERSION}
PUBLIC_INC_PATH=${INDEX_WORKSPACE}/public_inc
MONGO_JAVA_DRIVER=${INDEX_WORKSPACE}/src/mongo-driver
AZ_SWITCH=""
TCMALLOC_PATH=${MONGO_PATH}/src/third_party/gperftools-2.5
BACKUP_CHECK_TOOL_PATH=${ROCKSDB_PATH}/store_mgmt/

# print some variable
echo RELEASE_DIR=${RELEASE_DIR}
echo SRC_DIR=${SRC_DIR}
echo FTDS_PATH=${FTDS_PATH}
echo ROCKSDB_PATH=${ROCKSDB_PATH}
echo MONGOROCKS_PATH=${MONGOROCKS_PATH}
echo MONGO_PATH=${MONGO_PATH}
echo CPU_CORES=${CPU_CORES}

function SetMongoDBModule
{    
    #    If argument is set and not an option
    if [[ ( -n "$1" ) && ! ( "$1" == --* ) ]]; then
        if [[ $1 == "mongod" ]] || [[ $1 == "mongos" ]] || [[ $1 == "mongo" ]];then
            mongodb_module=$1
            log "INFO: compile mongo module ${mongodb_module}"
        else
            log "ERROR: invalid mongo module name ${mongodb_module}!"
            exit ${COMPILE_ERROR}
        fi
    fi
}

function SetGoal
{
    local goal=$1
    
    case $goal in
        ftds)
            SetGoal "index_base"
            ;;

        rocksdb)
            SetGoal "lz4"
            SetGoal "index_base"
            SetGoal "plog_client"
            SetGoal "rocksdb_only"
            SetGoal "db_bench"
            SetGoal "db_recovery_tool"
            SetGoal "db_rescue_tool"
            SetGoal "backup_check_tool"
            ;;

        mongodb)
            SetGoal "index_base"
            SetGoal "tcmalloc"
            SetGoal "mongodb_only"
            ;;

        compmonitor)
            SetGoal "index_base"
            SetGoal "compmonitor_only"
            ;;
            
        az)
            AZ_SWITCH="az"
            PCLIENTX_DIR=${BUILD_DIR}/index_public_az/repo/lib
            ZK_CLI_JAR=${BUILD_DIR}/index_public_az/repo/zk_jlib
            JDK_DIR=${BUILD_DIR}/index_public_az/repo/j2sdk-image        
            SetGoal "all"
            ;;

        all)
            SetGoal "rocksdb"
            SetGoal "mongodb"
            SetGoal "compmonitor"
            ;;
            
        lz4|index_base|plog_client|rocksdb_only|db_bench|db_recovery_tool|db_rescue_tool|backup_check_tool|tcmalloc|mongodb_only|compmonitor_only)
            goalMap[$goal]=1
            ;;

        *)
            echo "Unknown build goal: $goal"
            exit 1
            ;;
    esac
}

function IsGoalSet
{
    if [ ${goalMap[$1]+_} ]; then
        return 0;
    else
        return 1;
    fi
}


#
# Parse optional arguments: currently we support only --no_clean and --no_opt
#
for arg in "$@"
do
    case $arg in
        --no_clean)
            NO_CLEAN=true
            echo "NO_CLEAN=true (--no_clean). Don't clean existing binaries."
            ;;

        --no_opt)
            OPT_SWITCH=off
            echo "OPT_SWITCH=off (--no_opt). Don't optimize binaries, so they can be debuggable. NOTE: don't use this build on production."
            ;;        

        --*)
            echo "Unknown option of build_mongodb_mock: $arg"
            exit 1
            ;;
    esac
done


#    Goal map is the set of build goals to execute
declare -A goalMap

#    Set default MongoDB module to build. "core" means build both MongoS and MongoD
mongodb_module="core"

#
#    if we have goals set (first argument is no option) lets parse them
#
if [[ ( -n "$1" ) && ! ( "$1" == --* ) ]]; then
    IFS=', ' read -r -a argumentsArray <<< "$1"
    
    for element in "${argumentsArray[@]}"
    do
        SetGoal $element
    done    
else
    SetGoal "all"
fi

#    Just to support old argument format: if goal is "mongodb" it's possible to set a module to build as a second argument
if [[ $1 == "mongodb" ]] ; then
    SetMongoDBModule $2
fi

#
#    Print and execute requested goals
#
echo "Goals to build":
for element in "${!goalMap[@]}"
do
    echo "Goal: $element"    
done

if IsGoalSet "lz4"; then    
    compile_lz4
fi

if IsGoalSet "index_base"; then    
    compile_index_base
fi

if IsGoalSet "plog_client"; then    
    compile_plog_client "mock"
fi

if IsGoalSet "rocksdb_only"; then    
    compile_rocksdb_only
fi

if IsGoalSet "db_bench"; then
    compile_db_bench
fi

if IsGoalSet "db_recovery_tool"; then
    compile_db_recovery_tool
fi

if IsGoalSet "db_rescue_tool"; then
    compile_db_rescue_tool
fi

if IsGoalSet "backup_check_tool"; then
    compile_backup_check_tool "mock"
fi

if IsGoalSet "tcmalloc"; then    
    compile_tcmalloc
fi

if IsGoalSet "mongodb_only"; then    
    compile_mongodb_only ${mongodb_module} "mock"
fi

if IsGoalSet "compmonitor_only"; then    
    compile_compmonitor
fi

BUILD_END=$(date +%s)
BUILD_TIME=$((BUILD_END-BUILD_START))

printf 'Build succeeded. Time elapsed:  %dh:%dm:%ds\n' $(($BUILD_TIME/3600)) $(($BUILD_TIME%3600/60)) $(($BUILD_TIME%60))

### normal exit
echo "${AZ_SWITCH}" > ${SCRIPT_PATH}/az_flag
exit 0
