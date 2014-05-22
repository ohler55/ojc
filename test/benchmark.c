/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <stdio.h>
#include <sys/time.h>

#include "ojc.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static bool
each_cb(ojcVal val, void *ctx) {
    *((int64_t*)ctx) = *((int64_t*)ctx) + 1;
    return true;
}

int
main(int argc, char **argv) {
    FILE		*f;
    struct _ojcErr	err;
    int64_t		cnt = 0;
    int64_t		dt;
    int64_t		start = clock_micro();

    f = fopen("log.json", "r");
    ojc_err_init(&err);
    ojc_parse_stream(&err, f, each_cb, &cnt);
    fclose(f);
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("%lld entries in %0.3f msecs. (%g iterations/msec)\n",
	   cnt, (double)dt / 1000.0, (double)cnt * 1000.0 / (double)dt);

    return 0;
}
