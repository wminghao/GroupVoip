cd ../../
scons type=debug
cd mixcoder/test
cat $1 | ../../build/Linux-x86_64/mixcoder/prog/seg_input_parser input.flv