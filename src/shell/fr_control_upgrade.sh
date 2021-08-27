#! /bin/sh

echo "##### Uncompress fr_control upgrade file";
tar -zxvf /tmp/software/fr_control.tar.gz -C /tmp/

echo "##### CP fr_control upgrade file"
cp /tmp/software/README_CTL.txt /root/README/
# update QNX system image (xxx.ifs)
#if test -x /tmp/fr_control/frsys*.ifs; then
#	rm  /.boot/frsys*.ifs
#	cp  /tmp/fr_control/frsys*.ifs  /.boot/
#fi

# update web_recover
if [ -d /tmp/web_recover ]; then
	rm -rf /root/web_recover
	mv /tmp/web_recover /root/
fi

# update frapploader.sh
if test -x /tmp/fr_control/frapploader.sh; then
	rm  /etc/rc.d/frapploader.sh
	cp  /tmp/fr_control/frapploader.sh  /etc/rc.d/
fi

# update ethercat cobot_v2.xml
if test -x /tmp/fr_control/cobot_v2.xml; then
	rm  /root/ethercat/cobot_v2.xml;
	cp  /tmp/fr_control/cobot_v2.xml  /root/ethercat/
fi

# update control firmware and config files
if test -x /tmp/fr_control/zqrobot-v2.0; then
	rm  /root/robot/zqrobot-v2.0
	cp  /tmp/fr_control/zqrobot-v2.0  /root/robot/
fi

#if test -x /tmp/fr_control/zqrobotSim-v2.0; then
#	rm  /root/robot/zqrobotSim-v2.0
#	cp  /tmp/fr_control/zqrobotSim-v2.0  /root/robot/
#fi

#if test -x /tmp/fr_control/user.config; then
#	rm  /root/robot/user.config
#	cp  /tmp/fr_control/user.config  /root/robot/
#fi

if test -x /tmp/fr_control/zbt.config; then
	rm  /root/robot/zbt.config
	cp  /tmp/fr_control/zbt.config  /root/robot/
fi

if [ ! -f /tmp/fr_control/robot.config ]; then
	cp  /tmp/fr_control/robot.config  /root/robot/
fi

#if test -x /tmp/fr_control/ex_device.config; then
#	rm  /root/robot/ex_device.config
#	cp  /tmp/fr_control/ex_device.config  /root/robot/
#fi

#if test -x /tmp/fr_control/exaxis.config; then
#	rm  /root/robot/exaxis.config
#	cp  /tmp/fr_control/exaxis.config  /root/robot/
#fi

#if test -x /tmp/fr_control/userSim.config; then
#	rm  /root/robot/userSim.config
#	cp  /tmp/fr_control/userSim.config  /root/robot/
#fi

#if test -x /tmp/fr_control/zbtSim.config; then
#	rm  /root/robot/zbtSim.config
#	cp  /tmp/fr_control/zbtSim.config  /root/robot/
#fi

#update lib
if test -x /tmp/fr_control/liblua.so; then
	rm  /lib/liblua.so
	cp  /tmp/fr_control/liblua.so  /lib/
fi

#if test -x /tmp/fr_control/libmkpaiodev.so; then
#	rm  /lib/libmkpaiodev.so
#	cp  /tmp/fr_control/libmkpaiodev.so  /lib/
#fi

if test -x /tmp/fr_control/libqbothal.so; then
	rm  /lib/libqbothal.so
	cp  /tmp/fr_control/libqbothal.so  /lib/
fi

if test -x /tmp/fr_control/libqbotmath.so; then
	rm  /lib/libqbotmath.so
	cp  /tmp/fr_control/libqbotmath.so  /lib/
fi

if test -x /tmp/fr_control/libqbotmotioncontrol.so; then
	rm  /lib/libqbotmotioncontrol.so
	cp  /tmp/fr_control/libqbotmotioncontrol.so  /lib/
fi

#if test -x /tmp/fr_control/libqbotmotioncontrolSim.so; then
#	rm  /lib/libqbotmotioncontrolSim.so
#	cp  /tmp/fr_control/libqbotmotioncontrolSim.so  /lib/
#fi

if test -x /tmp/fr_control/libqbotsmoothtrajectory.so; then
	rm  /lib/libqbotsmoothtrajectory.so 
	cp  /tmp/fr_control/libqbotsmoothtrajectory.so  /lib/
fi

#if test -x /tmp/fr_control/librpcserver.so; then
#	rm  /lib/librpcserver.so
#	cp  /tmp/fr_control/librpcserver.so  /lib/
#fi
#if test -x /tmp/fr_control/lsm-ethercat.so; then
#	rm  /lib/dll/lsm-ethercat.so
#	cp  /tmp/fr_control/lsm-ethercat.so  /lib/dll/
#fi

echo "##### Upload fr_control upgrade file success!"

#echo "##### rm temporary file in the TMP directory"
#rm -rf /tmp/fr_control

# 创建标志 “升级成功” 的文件
touch /root/web/file/upgrade_success.txt

echo "#### sleep 15 "

# 文件写入硬盘需要一定时间，等待 15 秒
sleep 15

echo "##### need shutdown!"

wait
#shutdown -b
