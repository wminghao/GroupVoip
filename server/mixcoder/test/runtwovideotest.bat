cd ../../
scons type=debug
cd mixcoder/test
rm abc*.flv
g++ -ggdb -g3 -o twoflvtest twoflvtest.cpp flvrealtimeparser.cpp
./twoflvtest test.flv test3.flv| ../../build/Linux-x86_64/mixcoder/prog/mix_coder | ../../build/Linux-x86_64/mixcoder/prog/seg_output_parser abc
