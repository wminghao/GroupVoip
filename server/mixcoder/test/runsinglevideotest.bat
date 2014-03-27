cd ../../
scons type=debug
cd mixcoder/test
gcc -o singleflvtest singleflvtest.cpp
./singleflvtest test.flv| ../../build/Linux-x86_64/mixcoder/prog/mix_coder > abc.flv
