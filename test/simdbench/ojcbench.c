#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "ojc/buf.h"
#include "ojc/ojc.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static int
ojc_parse(const char *str, int64_t iter) {
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		dt;
    int64_t		start = clock_micro();
    ojcVal		ojc;

    for (int i = iter; 0 < i; i--) {
	ojc = ojc_parse_str(&err, str, NULL, NULL);
	ojc_destroy(ojc);
    }
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_parse_str    %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    return 0;
}
int
main(int argc, char **argv) {
    int64_t	iter = 1000000LL;
    const char	*str = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1}";

    ojc_parse(str, iter);

    return 0;
}
