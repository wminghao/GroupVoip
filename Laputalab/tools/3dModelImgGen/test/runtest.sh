#!/bin/bash
cd ..
scons
cd test
./runpanaroma.sh purpleglasses
./runpanaroma.sh RanGlasses
./runpanaroma.sh blackglasses
