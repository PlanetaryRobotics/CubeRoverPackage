#!/bin/sh
# **********************************************************************

sudo updatedb
locate $PWD/*tiR4-ti-cgt-arm-bin/*.a > tmp.txt
mkdir -p Libs
xargs -a tmp.txt cp -v -t Libs/ 
rm -f tmp.txt