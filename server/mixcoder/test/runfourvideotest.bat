cd ../../
scons type=debug
cd mixcoder/test
rm abc*.flv
g++ -o fourflvtest fourflvtest.cpp flvrealtimeparser.cpp
./fourflvtest test.flv test2.flv test3.flv test4.flv| ../../build/Linux-x86_64/mixcoder/prog/mix_coder | ../../build/Linux-x86_64/mixcoder/prog/seg_output_parser abc