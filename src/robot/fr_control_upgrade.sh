#! /bin/bash

echo "##### Uncompress fr_control upgrade file";
tar -zxvf /tmp/control/fr_control.tar.gz -C /tmp/
mv /tmp/control/README_CTL.txt /root/README/

echo "##### Mv fr_control upgrade file"
# update QNX system image (xxx.ifs)
if test -x /tmp/fr_control/frsys*.ifs; then
	rm  /.boot/frsys*.ifs
	cp  /tmp/fr_control/frsys*.ifs  /.boot/
fi

# update control firmware and config files
if test -x /tmp/fr_control/zqrobot-v2.0; then
	rm  /root/robot/zqrobot-v2.0
	cp  /tmp/fr_control/zqrobot-v2.0  /root/robot/
fi

if test -x /tmp/fr_control/zqrobotSim-v2.0; then
	rm  /root/robot/zqrobotSim-v2.0
	cp  /tmp/fr_control/zqrobotSim-v2.0  /root/robot/
fi

if test -x /tmp/fr_control/user.config; then
	rm  /root/robot/user.config
	cp  /tmp/fr_control/user.config  /root/robot/
fi

if test -x /tmp/fr_control/zbt.config; then
	rm  /root/robot/zbt.config
	cp  /tmp/fr_control/zbt.config  /root/robot/
fi

if test -x /tmp/fr_control/userSim.config; then
	rm  /root/robot/userSim.config
	cp  /tmp/fr_control/userSim.config  /root/robot/
fi

if test -x /tmp/fr_control/zbtSim.config; then
	rm  /root/robot/zbtSim.config
	cp  /tmp/fr_control/zbtSim.config  /root/robot/
fi

#update lib
if test -x /tmp/fr_control/liblua.so; then
	rm  /lib/liblua.so
	cp  /tmp/fr_control/liblua.so  /lib/
fi

if test -x /tmp/fr_control/libmkpaiodev.so; then
	rm  /lib/libmkpaiodev.so
	cp  /tmp/fr_control/libmkpaiodev.so  /lib/
fi

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

if test -x /tmp/fr_control/libqbotmotioncontrolSim.so; then
	rm  /lib/libqbotmotioncontrolSim.so
	cp  /tmp/fr_control/libqbotmotioncontrolSim.so  /lib/
fi

if test -x /tmp/fr_control/libqbotsmoothtrajectory.so; then
	rm  /lib/libqbotsmoothtrajectory.so 
	cp  /tmp/fr_control/libqbotsmoothtrajectory.so  /lib/
fi

if test -x /tmp/fr_control/librpcserver.so; then
	rm  /lib/librpcserver.so
	cp  /tmp/fr_control/librpcserver.so  /lib/
fi
if test -x /tmp/fr_control/lsm-ethercat.so; then
	rm  /lib/dll/lsm-ethercat.so
	cp  /tmp/fr_control/lsm-ethercat.so  /lib/dll/
fi

echo "##### rm temporary file in the TMP directory"
rm -rf /tmp/control
rm -f /tmp/control.tar.gz
rm -rf /tmp/fr_control

echo "##### Upload fr_control upgrade file success!"

echo "##### will shutdown!"
#shutdown
