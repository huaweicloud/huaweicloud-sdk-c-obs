#!/bin/bash

set -x

WORKSPACE_LOG=$1
WORKSPACE=/home/workspace/esdk_obs_c_uniautos_test
UNIAUTOS_ROOT_DIR=$WORKSPACE/uniautostest/codes
UNIAUTOS_BIN_DIR=$UNIAUTOS_ROOT_DIR/UniAutos/src/Framework/Dev/bin
UNIAUTOS_CONFIG_DIR=$UNIAUTOS_BIN_DIR/Config
UNIAUTOS_LOGS_DIR=$WORKSPACE_LOG/uniautos_logs
DFV_TEST_ROOT_DIR=$WORKSPACE/dfvtest
MAIN_CONFIG_FILE=$DFV_TEST_ROOT_DIR/sdk/config/mainConfig_CI.xml
TESTBED_FILE=$DFV_TEST_ROOT_DIR/sdk/config/testBedInfo_CI.xml
TESTSET_FILE=$DFV_TEST_ROOT_DIR/sdk/config/testSetInfo_CI.xml

if [ ! -d "$UNIAUTOS_ROOT_DIR" ]; then
    mkdir -p "$UNIAUTOS_ROOT_DIR"
fi

# Download the UniAutos framework
UNIAUTOS_DIR=$UNIAUTOS_ROOT_DIR/UniAutos
cd $UNIAUTOS_ROOT_DIR
if [ ! -d "$UNIAUTOS_DIR" ]; then
    git clone -b develop_1016 http://10.183.61.55/oceanstor-autotest/UniAutos.git
fi
cd $UNIAUTOS_DIR
git pull    

# Download the testing scripts
DFVTEST_DIR=$WORKSPACE/dfvtest
cd $WORKSPACE
if [ ! -d "$DFVTEST_DIR" ]; then
    git clone http://code.huawei.com/DFV-Test-Group/dfvtest.git
fi
cd $DFVTEST_DIR
git pull

#git clone http://git@code-sh.huawei.com/d00417089/dfvtest.git

#保证testBed中的ip为当前环境的ip
yes | cp /home/config/testbed/testBedInfo_CI.xml $TESTBED_FILE
# Delete the history logs
rm -rf $UNIAUTOS_LOGS_DIR/*

# Run test cases
python $UNIAUTOS_BIN_DIR/UniAutosScript.py --configFile $MAIN_CONFIG_FILE -tb $TESTBED_FILE -ts $TESTSET_FILE -le $UNIAUTOS_LOGS_DIR
