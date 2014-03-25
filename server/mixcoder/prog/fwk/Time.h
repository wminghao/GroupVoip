
#include <time.h>



struct timeval tv;


gettimeofday(&tv, NULL);

unsigned long long millisecondsSinceEpoch =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;

printf("%llu\n", millisecondsSinceEpoch);
