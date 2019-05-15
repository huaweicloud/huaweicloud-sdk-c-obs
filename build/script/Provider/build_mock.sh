#!/bin/sh

#execute
currdir=$(dirname $0)
az_switch=""
if [[ $1 == "az" ]];then
    az_switch="az"
fi
echo "execute: sh ${currdir}/build_mongodb_mock.sh ${az_switch}"
sh ${currdir}/build_mongodb_mock.sh "${az_switch}" 

