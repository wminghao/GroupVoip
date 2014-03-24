cd ../../
scons type=debug
cd mixcoder/test
gcc -o twoflvtest twoflvtest.cpp
./twoflvtest test.flv| ../../build/Linux-x86_64/mixcoder/prog/mix_coder > abc.flv
