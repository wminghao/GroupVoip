#!/bin/bash
pwd=`pwd`
curdir=$pwd"/../"
modeldir=$curdir'3dmodels'
cd $modeldir
if [ ! -d "result" ];
then
   mkdir result
fi
cd result
rm $1.tar.gz 
rm -rf $1
cd $pwd
./runsingle.sh $1 +0 $pwd $modeldir $curdir
./runsingle.sh $1 +10 $pwd $modeldir $curdir
./runsingle.sh $1 +20 $pwd $modeldir $curdir
./runsingle.sh $1 +30 $pwd $modeldir $curdir
./runsingle.sh $1 +40 $pwd $modeldir $curdir
./runsingle.sh $1 +50 $pwd $modeldir $curdir
./runsingle.sh $1 +60 $pwd $modeldir $curdir
./runsingle.sh $1 +70 $pwd $modeldir $curdir
./runsingle.sh $1 +80 $pwd $modeldir $curdir
./runsingle.sh $1 +90 $pwd $modeldir $curdir
./runsingle.sh $1 +100 $pwd $modeldir $curdir
./runsingle.sh $1 +110 $pwd $modeldir $curdir
./runsingle.sh $1 +120 $pwd $modeldir $curdir
./runsingle.sh $1 +130 $pwd $modeldir $curdir
./runsingle.sh $1 +140 $pwd $modeldir $curdir
./runsingle.sh $1 +150 $pwd $modeldir $curdir
./runsingle.sh $1 +160 $pwd $modeldir $curdir
./runsingle.sh $1 +170 $pwd $modeldir $curdir
./runsingle.sh $1 +180 $pwd $modeldir $curdir
./runsingle.sh $1 -10 $pwd $modeldir $curdir
./runsingle.sh $1 -20 $pwd $modeldir $curdir
./runsingle.sh $1 -30 $pwd $modeldir $curdir
./runsingle.sh $1 -40 $pwd $modeldir $curdir
./runsingle.sh $1 -50 $pwd $modeldir $curdir
./runsingle.sh $1 -60 $pwd $modeldir $curdir
./runsingle.sh $1 -70 $pwd $modeldir $curdir
./runsingle.sh $1 -80 $pwd $modeldir $curdir
./runsingle.sh $1 -90 $pwd $modeldir $curdir
./runsingle.sh $1 -100 $pwd $modeldir $curdir
./runsingle.sh $1 -110 $pwd $modeldir $curdir
./runsingle.sh $1 -120 $pwd $modeldir $curdir
./runsingle.sh $1 -130 $pwd $modeldir $curdir
./runsingle.sh $1 -140 $pwd $modeldir $curdir
./runsingle.sh $1 -150 $pwd $modeldir $curdir
./runsingle.sh $1 -160 $pwd $modeldir $curdir
./runsingle.sh $1 -170 $pwd $modeldir $curdir
cd $modeldir/result
tar -zcvf $1.tar.gz $1
cd $pwd
