#!/bin/sh

LG_ID1=`ps -fea | grep lguest | awk 'NR==1{printf($2)}'`
LG_ID2=`ps -fea | grep lguest | awk 'NR==2{printf($2)}'`
LG_ID3=`ps -fea | grep lguest | awk 'NR==3{printf($2)}'`

echo "Please wait..."

echo "Killing lguest. PID=$LG_ID"
sudo kill -9 $LG_ID1
sudo kill -9 $LG_ID2
sudo kill -9 $LG_ID3


# if it refuses to close, uncomment this lines
#sudo kill -INT $LG_ID
#sudo kill -HUP $LG_ID
#sudo kill -KILL $LG_ID
