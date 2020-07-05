#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <ostream>

#include "simdjson.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static int
simd_parse(const char *str, int64_t iter) {
    simdjson::dom::parser	parser;
    int64_t			dt;
    int64_t			start = clock_micro();

    for (int i = iter; 0 < i; i--) {
	simdjson::dom::element	doc = parser.parse(str, strlen(str));
    }
    dt = clock_micro() - start;

    printf("simdjson_parse   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
}

int
main(int argc, char **argv) {
    int64_t	iter = 1000000LL;

    const char	*str = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1}";

    simd_parse(str, iter);

    return 0;
}
