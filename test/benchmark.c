/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "ojc/ojc.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static bool
each_cb(ojcErr err, ojcVal val, void *ctx) {
    *((int64_t*)ctx) = *((int64_t*)ctx) + 1;
    return true;
}

static int
bench_write(const char *filename, int64_t iter) {
    FILE		*f;
    struct _ojcErr	err;
    int64_t		i;
    int64_t		dt;
    int64_t		start = clock_micro();
    ojcVal		obj;
    int			fd;

    obj = ojc_create_object();
    ojc_object_append(&err, obj, "level", ojc_create_str("INFO", 0));
    ojc_object_append(&err, obj, "message",
		      ojc_create_str("This is a log message that is long enough to be representative of an actual message.", 0));
    ojc_object_append(&err, obj, "msgType", ojc_create_int(1));
    ojc_object_append(&err, obj, "source", ojc_create_str("Test", 0));
    ojc_object_append(&err, obj, "thread", ojc_create_str("main", 0));
    ojc_object_append(&err, obj, "timestamp", ojc_create_int(1400000000000000000LL));
    ojc_object_append(&err, obj, "version", ojc_create_int(1));

    start = clock_micro();
    ojc_err_init(&err);
    f = fopen(filename, "w");
    fd = fileno(f);
    for (i = 0; i < iter; i++) {
	ojc_write(&err, obj, 0, fd);
	if (write(fd, "\n", 1)) {}
    }
    fclose(f);
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_write      %lld entries in %0.3f msecs. (%g iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (double)iter * 1000.0 / (double)dt);
    return 0;
}

static int
bench_read(const char *filename) {
    FILE		*f;
    struct _ojcErr	err;
    int64_t		cnt = 0;
    int64_t		dt;
    int64_t		start = clock_micro();

    f = fopen(filename, "r");
    ojc_err_init(&err);
    ojc_parse_file(&err, f, each_cb, &cnt);
    fclose(f);
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_parse_file %lld entries in %0.3f msecs. (%g iterations/msec)\n",
	   (long long)cnt, (double)dt / 1000.0, (double)cnt * 1000.0 / (double)dt);

    return 0;
}

static int
bench_parse(const char *filename, int64_t iter) {
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		dt;
    ojcVal		obj;

    obj = ojc_create_object();
    ojc_object_append(&err, obj, "level", ojc_create_str("INFO", 0));
    ojc_object_append(&err, obj, "message",
		      ojc_create_str("This is a log message that is long enough to be representative of an actual message.", 0));
    ojc_object_append(&err, obj, "msgType", ojc_create_int(1));
    ojc_object_append(&err, obj, "source", ojc_create_str("Test", 0));
    ojc_object_append(&err, obj, "thread", ojc_create_str("main", 0));
    ojc_object_append(&err, obj, "timestamp", ojc_create_int(1400000000000000000LL));
    ojc_object_append(&err, obj, "version", ojc_create_int(1));

    char	*str = ojc_to_str(obj, 0);
    int64_t	start = clock_micro();
    ojcVal	ojc;

    for (int i = iter; 0 < i; i--) {
	ojc = ojc_parse_str(&err, str, NULL, NULL);
	ojc_destroy(ojc);
    }
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_parse_str  %lld entries in %0.3f msecs. (%g iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (double)iter * 1000.0 / (double)dt);

    return 0;
}

int
main(int argc, char **argv) {
    const char	*filename = "log.json";
    int64_t	iter = 100000LL;

    bench_write(filename, iter);
    bench_read(filename);
    bench_parse(filename, iter);

    return 0;
}
