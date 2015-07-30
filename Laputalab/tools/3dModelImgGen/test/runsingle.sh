#!/bin/bash
cd $4/result
if [ ! -d "$1" ];
then
   mkdir $1
fi
cd $3
cd ..
build/Linux-x86_64/VET/prog/3dModelImgGen white.jpg $1$2.jpg $1 $2 $5
cd test
