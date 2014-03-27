cd ../../
scons type=debug
cd mixcoder/test
gcc -o twoflvtest twoflvtest.cpp
./twoflvtest test.flv test2.flv| ../../build/Linux-x86_64/mixcoder/prog/mix_coder | ../../build/Linux-x86_64/mixcoder/prog/seg_output_parser abc
