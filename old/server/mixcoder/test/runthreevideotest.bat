cd ../../
scons type=debug
cd mixcoder/test
rm abc*.flv
g++ -o threeflvtest threeflvtest.cpp flvrealtimeparser.cpp
./threeflvtest test.flv test2.flv test3.flv | ../../build/Linux-x86_64/mixcoder/prog/mix_coder | ../../build/Linux-x86_64/mixcoder/prog/seg_output_parser abc
