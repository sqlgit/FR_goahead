#!/bin/sh

echo "##### Do web upgrade"
NAME="goahead"    #想要杀死的进程
CLIENTIP=$1   #client ip
ID=`ps -ef | grep "$NAME" | grep -v "grep" | awk '{print $2}'`  #注意此shell脚本的名称，避免自杀
if [ -z "$ID" ];then
	echo "##### process id is empty, process is not existed..."
	echo "##### process will start..."
		cd /root/web/webserver && ./goahead /root/web/frontend $CLIENTIP:80 &
	echo "##### process has start..."
else
#	echo $ID
	for id in $ID
	do
		kill -9 $id
		echo "##### killed $id"
	done
	echo "##### process will restart..."
		cd /root/web/webserver && ./goahead /root/web/frontend $CLIENTIP:80 &
	echo "##### process has restart..."
fi
#shutdown now &

echo "##### WebAPP upgrade success!"

wait
