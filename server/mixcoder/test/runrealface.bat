cd ../../
scons type=debug
cd mixcoder/test
rm abc*.flv
cat realface.seg | ../../build/Linux-x86_64/mixcoder/prog/mix_coder > realface_output.seg
cat realface_output.seg | ../../build/Linux-x86_64/mixcoder/prog/seg_output_parser abc

