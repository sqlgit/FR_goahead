#!/bin/sh
#项目的主干目录是相同的
RELEASE="/root/web/log"

#for i in "$*"; do
#	echo $i
#done

echo $1

#拼接文件路径
releasepath=${RELEASE}
cd $releasepath
#判断是否存在该目录
if [ $? -eq 0 ];then
	echo $releasepath
	echo "Contains file:"
	#输出所有的内容
	echo *
	num=`ls -l | grep '^-' | wc -l`;
	echo $num
	#判断文件的数量是否超过 $1 个（我只想保留最新的 $1 个文件）
	if [ $num -gt $1 ];then
		#计算超过 $1 个多少
		num=`expr $num - $1`
		clean=`ls -tr 2* | head -$num | xargs`
		echo "will delete file:"
		echo ${clean}
		#-n1 每次处理1个文件
		ls -tr 2* | head -$num | xargs -i -n1 rm -rf {}
	fi
fi
