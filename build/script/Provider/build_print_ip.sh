#!/bin/sh

#get the ip address of this host
ip_addr=`/sbin/ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6|awk '{print $2}'|tr -d "addr:"`
echo "------------- This task runs on the host $ip_addr"
echo "------------- Current user HOME is $HOME"
echo "------------- Current os PATH is $PATH , ccache is `which ccache`"
