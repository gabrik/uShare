#!/bin/sh
# 
# File:   start.sh.sh
# Author: gabriele
#
# Created on 3-lug-2017, 15.13.39
#

#"${OUTPUT_PATH}"  -c /home/gabriele/Video -v -d -i eth0 -a 172.16.0.8

#mount /home/gabriele/Recorded
echo "Personal is at $1"
./src/ushare -c /home/gabriele/Video/Recorded -v -d -i eth0 -a $1
#umount /home/gabriele/Recorded
