/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "ojc/buf.h"
#include "ojc/ojc.h"
#include "oj/oj.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

#if 0
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
	//if (write(fd, "\n", 1)) {}
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
bench_read(const char *filename, int64_t iter) {
    FILE		*f;
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		cnt = 0;
    int64_t		dt;
    int64_t		start = clock_micro();

    ojc_err_init(&err);
    f = fopen(filename, "r");
    ojc_parse_file(&err, f, each_cb, &cnt);
    /*
    for (int i = iter; 0 < i; i--) {
	ojc_parse_file(&err, f, each_cb, &cnt);
	fseek(f, 0, SEEK_SET);
    }
    */
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
#endif

static const char	json[] = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";

static int
bench_parse(const char *filename, int64_t iter) {
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		dt;
    ojcVal		ojc;
    const char		*str = json;
    char		*buf = NULL;

    if (NULL != filename) {
	FILE	*f = fopen(filename, "r");
	long	len;

	if (NULL == f) {
	    printf("*-*-* failed to open file %s\n", filename);
	    exit(1);
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (NULL == (buf = malloc(len + 1))) {
	    printf("*-*-* not enough memory to load file %s\n", filename);
	    exit(1);
	}
	if (len != fread(buf, 1, len, f)) {
	    printf("*-*-* reading file %s failed\n", filename);
	    exit(1);
	}
	buf[len] = '\0';
	fclose(f);
	str = buf;
    }

    int64_t	start = clock_micro();

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

    struct _ojErr	e = OJ_ERR_INIT;

    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	if (OJ_OK != oj_validate_str(&e, str)) {
	    break;
	}
    }
    dt = clock_micro() - start;
    if (OJ_OK != e.code) {
	printf("*** Error: %s at %d:%d\n", e.msg, e.line, e.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_validate_str   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    struct _ojParser	p;
    memset(&p, 0, sizeof(p));
    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	oj_parser_reset(&p);
	if (OJ_OK != oj_parse_str(&p, str)) {
	    break;
	}
    }
    dt = clock_micro() - start;
    if (OJ_OK != e.code) {
	printf("*** Error: %s at %d:%d\n", e.msg, e.line, e.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_parse_str   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

int
main(int argc, char **argv) {
    const char	*filename = "log.json";
    int64_t	iter = 1000000LL;

    if (1 < argc) {
	filename = argv[1];
	//bench_read(filename, iter);
	bench_parse(filename, iter / 100);
	return 0;
    }
    //bench_fill(iter);
    //bench_write(filename, iter);
    //bench_read(filename, iter);
    bench_parse(NULL, iter);

    return 0;
}
