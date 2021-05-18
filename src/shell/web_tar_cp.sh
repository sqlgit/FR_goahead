#!/bin/sh

echo "##### CP web upgrade file"

echo "##### Upload web upgrade file success!"
cp /tmp/software/README_WEB.txt /root/README/

#tar -zxvf /tmp/webapp/web.tar.gz -C /root/

# update webserver
rm -rf /root/web/webserver/
cp -R /tmp/web/webserver /root/web/

sleep 1

# update frontend
cd /root/web/frontend/ && rm -rf !(file) && cd -
cp -R /tmp/web/frontend /root/web/

# update system
#sleep 1
#rm -rf /root/web/file/cfg/
#cp -R /tmp/web/file/cfg /root/web/file/

#echo "##### rm temporary file in the TMP directory"
#rm -rf /tmp/web
