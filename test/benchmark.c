/*******************************************************************************
 * Copyright (c) 2014, 2020 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
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

static int
walk_oj(ojVal val) {
    int	cnt = 0;

    switch (val->type) {
    case OJ_NULL:
	cnt++;
	break;
    case OJ_TRUE:
	break;
    case OJ_FALSE:
	cnt++;
	break;
    case OJ_INT:
	if (0 == oj_val_get_int(val)) {
	    cnt++;
	}
	break;
    case OJ_DECIMAL:
	if (0.0 == oj_val_get_double(val)) {
	    cnt++;
	}
	break;
    case OJ_STRING:
	if ('\0' == *val->str.raw) {
	    cnt++;
	}
	break;
    case OJ_OBJECT:
	if (OJ_OBJ_RAW == val->mod) {
	    for (ojVal v = val->list.head; NULL != v; v = v->next) {
		cnt += walk_oj(v);
	    }
	} else {
	    ojVal	*b = val->hash;
	    ojVal	*bend = b + (sizeof(val->hash) / sizeof(*val->hash));
	    ojVal	v;

	    for (; b < bend; b++) {
		for (v = *b; NULL != v; v = v->next) {
		    cnt += walk_oj(v);
		}
	    }
	}
	break;
    case OJ_ARRAY:
	for (ojVal v = val->list.head; NULL != v; v = v->next) {
	    cnt += walk_oj(v);
	}
	break;
    }
    return cnt;
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
//static const char	json[] = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";

static bool
noop_cb(ojcErr err, ojcVal val, void *ctx) {
    return true;
}

static char*
load_file(const char *filename) {
    FILE	*f = fopen(filename, "r");
    long	len;
    char	*buf;

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

    return buf;
}

static int
bench_parse(const char *filename, int64_t iter) {
    int64_t		dt;
    const char		*str = json;
    char		*buf = NULL;

    if (NULL != filename) {
	buf = load_file(filename);
	str = buf;
    }
    struct _ojErr	e = OJ_ERR_INIT;
    int64_t		start = clock_micro();

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
    printf("oj_validate_str %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    ojVal	v;

    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	v = oj_val_parse_str(&e, str, NULL, NULL);
	oj_destroy(v);
    }
    dt = clock_micro() - start;
    if (OJ_OK != e.code) {
	printf("*** Error: %s at %d:%d\n", e.msg, e.line, e.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_parse_str    %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    struct _ojcErr	err = OJC_ERR_INIT;
    ojcVal		ojc;

    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	ojc = ojc_parse_str(&err, str, noop_cb, NULL);
	ojc_destroy(ojc);
    }
    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_parse_str   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

static bool
destroy_cb(ojVal val, void *ctx) {
    walk_oj(val);
    oj_destroy(val);
    *(long*)ctx = *(long*)ctx + 1;
    return true;
}

static int
bench_parse_many(const char *filename) {
    struct _ojcErr	err = OJC_ERR_INIT;
    int64_t		dt;
    ojcVal		ojc;
    const char		*str = json;
    char		*buf = NULL;
    long		iter = 0;
    struct _ojErr	e = OJ_ERR_INIT;

    //int64_t	start = clock_micro();

    if (NULL != filename) {
	//int64_t	t0 = clock_micro();
	buf = load_file(filename);
	str = buf;
	//printf("*** file loaded in %0.3f msec\n", (double)(clock_micro() - t0) / 1000.0);
    }
    int64_t	start = clock_micro();

    oj_val_parse_str(&e, str, destroy_cb, &iter);
    dt = clock_micro() - start;
    if (OJ_OK != e.code) {
	printf("*** Error: %s at %d:%d\n", e.msg, e.line, e.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_parse_str    %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    start = clock_micro();
    if (OJ_OK != oj_validate_str(&e, str)) {
	printf("*** Error: %s\n", e.msg);
	return -1;
    }

    dt = clock_micro() - start;
    if (OJ_OK != e.code) {
	printf("*** Error: %s at %d:%d\n", e.msg, e.line, e.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_validate_str %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    start = clock_micro();
    ojc = ojc_parse_str(&err, str, noop_cb, NULL);
    ojc_destroy(ojc);

    dt = clock_micro() - start;
    if (OJC_OK != err.code) {
	printf("*** Error: %s\n", err.msg);
	return -1;
    }
    printf("ojc_parse_str   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

static char*
mem_use(char *buf, size_t size) {
    struct rusage	usage;

    *buf = '\0';
    // TBD round to at least 2 places, adjust for linux vs macOS
    if (0 == getrusage(RUSAGE_SELF, &usage)) {
	long	mem = usage.ru_maxrss;
#ifndef __unix__
	mem /= 1024; // results are in KB, macOS in bytes
#endif
	if (1024 * 1024 * 10 < mem) {
	    snprintf(buf, size, "%ldGB", (mem + (1024 * 1024) / 2) / (1024 * 1024));
	} else if (1024 * 1024 < mem) {
	    snprintf(buf, size, "%0.1fGB", ((double)mem + (1024.0 * 1024.0) / 20.0) / (1024.0 * 1024.0));
	} else if (1024 * 10 < mem) {
	    snprintf(buf, size, "%ldMB", (mem + 512) / 1024);
	} else if (1024 < mem) {
	    snprintf(buf, size, "%0.1fMB", ((double)mem + 51.2) / 1024.0);
	} else {
	    snprintf(buf, size, "%ldKB", mem);
	}
    }
    return buf;
}

static int
bench_parse_file(const char *filename) {
    int64_t		dt;
    struct _ojParser	p;
    //struct depth_cnt	dc = { .depth = 0, .cnt = 0 };
    long		iter = 0;
    struct _ojErr	e = OJ_ERR_INIT;

    memset(&p, 0, sizeof(p));

    int64_t	start = clock_micro();

    oj_val_parse_file(&e, filename, destroy_cb, &iter);
    dt = clock_micro() - start;

    char	mem[16];
    if (OJ_OK != p.err.code) {
	printf("*** Error: %s at %d:%d\n", p.err.msg, p.err.line, p.err.col);
	//printf("*** Error: %s at %d:%d in %s\n", e.msg, e.line, e.col, str);
	return -1;
    }
    printf("oj_parse_file   %ld entries in %8.3f msecs. (%5d iterations/msec) used %s of memory\n",
	   iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt), mem_use(mem, sizeof(mem)));

    return 0;
}

extern void	debug_report();

int
main(int argc, char **argv) {
    const char	*filename = "log.json";
    int64_t	iter = 1000000LL;

   if (1 < argc) {
	filename = argv[1];

	bench_parse_file(filename);

	if (2 < argc) {
	    iter = strtoll(argv[2], NULL, 10);
	}
	//bench_read(filename, iter);
	//bench_parse(filename, iter / 10);
	if (1 == iter) {
	    bench_parse_many(filename);
	} else {
	    bench_parse(filename, iter);
	}
	return 0;
    }
    //bench_fill(iter);
    //bench_write(filename, iter);
    //bench_read(filename, iter);
    bench_parse(NULL, iter);

    oj_cleanup();
    debug_report();

    return 0;
}
