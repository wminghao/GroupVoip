mkfifo foo
cat slow.seg > foo &
valgrind --leak-check=yes ../../build/Linux-x86_64/mixcoder/prog/mix_coder < foo > slow_output.seg
