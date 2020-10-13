#!/bin/sh

echo "##### Uncompress tar web upgrade file"

rm -rf /root/web/webserver
rm -rf /root/web/frontend
#tar -zxvf /tmp/webapp/web.tar.gz -C /root/
mv /tmp/web/webserver /root/web/
mv /tmp/web/frontend /root/web/
mv /tmp/webapp/README_WEB.txt /root/README/

echo "##### rm temporary file in the TMP directory"
rm -f /tmp/webapp.tar.gz
rm -rf /tmp/webapp
rm -rf /tmp/web
