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

#if 1
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
	if (0 == val->num.fixnum) {
	    cnt++;
	}
	break;
    case OJ_DECIMAL:
	if (0.0 == val->num.dub) {
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
#endif

static const char	json[] = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";
//static const char	json[] = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";

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

static void
print_results(const char *name, int64_t iter, int64_t usec, ojErr err) {
    if (OJ_OK != err->code) {
	printf("%-18s: [%d] %s at %d:%d\n", name, err->code, err->msg, err->line, err->col);
    } else {
	double	per = (double)usec / (double)iter;

	printf("%-18s: %lld entries in %8.3f msecs, %7.2f usec/iterations  %7.1f iterations/msec\n",
	       name, (long long)iter, (double)usec / 1000.0, per, 1000.0 / per);
    }
}

static void
push_cb(ojVal val, void *ctx) {
/*
    if (3 == val->key.len && 0 == strcmp("alg", val->key.raw) && NULL != ctx) {
	*(long*)ctx = *(long*)ctx + 1;
    }
*/
}

static void
pop_cb(void *ctx) {
}

static int
bench_parse(const char *filename, int64_t iter) {
    int64_t		dt;
    const char		*str = json;
    char		*buf = NULL;
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		v;
    int64_t		start;
    struct _ojReuser	reuser = { .head = NULL, .tail = NULL, .dig = NULL };

    if (NULL != filename) {
	buf = load_file(filename);
	str = buf;
    }
    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	v = oj_parse_str_reuse(&err, str, &reuser);
	oj_reuse(&reuser);
    }
    dt = clock_micro() - start;
    print_results("oj_parse_str", iter, dt, &err);

    oj_err_init(&err);
    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	oj_pp_parse_str(&err, str, push_cb, pop_cb, NULL);
    }
    dt = clock_micro() - start;
    print_results("oj_pp_parse_str", iter, dt, &err);

    oj_err_init(&err);
    start = clock_micro();
    for (int i = iter; 0 < i; i--) {
	if (OJ_OK != oj_validate_str(&err, str)) {
	    break;
	}
    }
    dt = clock_micro() - start;
    print_results("oj_validate_str", iter, dt, &err);

    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

static ojCallbackOp
destroy_cb(ojVal val, void *ctx) {
    /*
    if (-1 == walk_oj(val)) {
	printf("dummy\n");
    }
    */
    *(long*)ctx = *(long*)ctx + 1;
    return OJ_DESTROY;
}

static ojCallbackOp
destroy_cbx(ojVal val, void *ctx) {
    for (int i = 0; 0 < i; i--) {
	if (-1 == walk_oj(val)) {
	    printf("dummy\n");
	}
    }
    *(long*)ctx = *(long*)ctx + 1;
    return OJ_DESTROY;
}

static int
bench_parse_many(const char *filename) {
    struct _ojErr	err = OJ_ERR_INIT;
    int64_t		dt;
    const char		*str = json;
    char		*buf = NULL;
    long		iter = 0;
    int64_t		file_load_time;
    int64_t		start;

    oj_thread_safe = true;

    printf("oj_parse_file includes file load time in results\n");
    start = clock_micro();
    oj_parse_file(&err, filename, destroy_cb, &iter);
    dt = clock_micro() - start;
    print_results("oj_parse_file", iter, dt, &err);

    start = clock_micro();
    if (NULL != filename) {
	int64_t	t0 = clock_micro();

	buf = load_file(filename);
	str = buf;
	printf("%s loaded in %0.3f msec\n", filename, (double)(clock_micro() - t0) / 1000.0);
    }
    file_load_time = clock_micro() - start;
    iter = 0;

    oj_err_init(&err);
    start = clock_micro();
    oj_parse_str(&err, str, destroy_cb, &iter);
    dt = clock_micro() - start;
    dt += file_load_time;
    print_results("oj_parse_str", iter, dt, &err);

    start = clock_micro();
    oj_pp_parse_str(&err, str, push_cb, pop_cb, NULL);
    dt = clock_micro() - start;
    dt += file_load_time;
    print_results("oj_pp_parse_str", iter, dt, &err);

    oj_err_init(&err);
    struct _ojCaller	caller;

    oj_err_init(&err);
    iter = 0;
    oj_caller_start(&err, &caller, destroy_cbx, &iter);
    start = clock_micro();
    oj_parse_str_call(&err, str, &caller);
    oj_caller_wait(&caller);
    dt = clock_micro() - start;
    dt += file_load_time;
    print_results("oj_parse_str_call", iter, dt, &err);

    oj_err_init(&err);
    start = clock_micro();
    oj_validate_str(&err, str);
    dt = clock_micro() - start;
    dt += file_load_time;
    print_results("oj_validate_str", iter, dt, &err);

    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

#if 0
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

    oj_val_parser_init(&p);

    int64_t	start = clock_micro();

    oj_parse_file(&p, filename, destroy_cb, &iter);
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
#endif

extern void	debug_report();

int
main(int argc, char **argv) {
    const char	*filename = "log.json";
    int64_t	iter = 1000000LL;

   if (1 < argc) {
	filename = argv[1];

	//bench_parse_file(filename);

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
	oj_cleanup();
	debug_report();
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
