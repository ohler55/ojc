/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "ojc/buf.h"
#include "ojc/ojc.h"
#include "ojc/wire.h"

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
bench_fill(int64_t iter) {
    struct _Buf		buf;
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		i;
    int64_t		dt;
    int64_t		start;
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

    start = clock_micro();
    for (i = 0; i < iter; i++) {
	buf_init(&buf, 0);
	ojc_buf(&buf, obj, 0, 0);
	buf_cleanup(&buf);
    }
    dt = clock_micro() - start;
    printf("ojc_fill        %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
    return 0;
}

static int
bench_write(const char *filename, int64_t iter) {
    FILE		*f;
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		i;
    int64_t		dt;
    int64_t		start;
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
    printf("ojc_write       %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
    return 0;
}

static int
bench_read(const char *filename) {
    FILE		*f;
    struct _ojcErr	err = OJC_ERR_INIT;
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
    printf("ojc_parse_file  %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)cnt, (double)dt / 1000.0, (int)((double)cnt * 1000.0 / (double)dt));

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
    printf("ojc_parse_str   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    return 0;
}

static int
bench_wire_fill(int64_t iter) {
    uint8_t		buf[1024];
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		i;
    int64_t		dt;
    int64_t		start;
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

    start = clock_micro();
    for (i = 0; i < iter; i++) {
	ojc_wire_fill(obj, buf, 0);
    }
    dt = clock_micro() - start;
    printf("ojc_wire_fill   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
    return 0;
}

static int
bench_wire_write(const char *filename, int64_t iter) {
    FILE		*f;
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		i;
    int64_t		dt;
    int64_t		start;
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
	ojc_wire_write_fd(&err, obj, fd);
    }
    fclose(f);
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_wire_fd     %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
    return 0;
}

static int
bench_wire_parse(int64_t iter) {
    uint8_t		buf[1024];
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		i;
    int64_t		dt;
    int64_t		start;
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

    ojc_wire_fill(obj, buf, 0);

    start = clock_micro();
    for (i = 0; i < iter; i++) {
	ojc_wire_parse(&err, buf);
    }
    dt = clock_micro() - start;
    printf("ojc_wire_parse  %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
    return 0;
}

int
main(int argc, char **argv) {
    const char	*filename = "log.json";
    int64_t	iter = 1000000LL;

    bench_fill(iter);
    bench_write(filename, iter);
    bench_read(filename);
    bench_parse(filename, iter);

    bench_wire_fill(iter);
    bench_wire_write(filename, iter);
    //bench_read(filename);
    bench_wire_parse(iter);

    return 0;
}
