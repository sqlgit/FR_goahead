#!/bin/sh

echo "##### CP web upgrade file"
cp /tmp/software/README_WEB.txt /root/README/
rm -rf /root/web/webserver
rm -rf /root/web/frontend
#tar -zxvf /tmp/webapp/web.tar.gz -C /root/
cp -R /tmp/web/webserver /root/web/
cp -R /tmp/web/frontend /root/web/
echo "##### Upload web upgrade file success!"

#echo "##### rm temporary file in the TMP directory"
#rm -rf /tmp/web
