// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj/oj.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
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
walk(ojVal val) {
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
	if (0.0 == oj_val_get_double(val, false)) {
	    cnt++;
	}
	break;
    case OJ_STRING:
	if ('\0' == *val->str.raw) {
	    cnt++;
	}
	break;
    case OJ_OBJECT:
    case OJ_ARRAY:
	for (ojVal v = val->list.head; NULL != v; v = v->next) {
	    cnt += walk(v);
	}
	break;
    }
    return cnt;
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
bench_parse(const char *filename, long long iter) {
    int64_t		dt;
    ojVal		v;
    struct _ojErr	err = OJ_ERR_INIT;
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();

    for (int i = iter; 0 < i; i--) {
	v = oj_val_parse_str(&err, buf, NULL, NULL);
	//walk(v);
	oj_destroy(v);
    }
    dt = clock_micro() - start;

    if (OJ_OK != err.code) {
	printf("{\"name\":\"oj\",\"err\":\"%s\"}\n", err.msg);
    } else {
	char	mem[16];

	printf("{\"name\":\"oj\",\"usec\":%lld,\"iter\":%lld,\"mem\":\"%s\"}\n",
	       (long long)dt, iter, mem_use(mem, sizeof(mem)));
    }
    if (NULL != buf) {
	free(buf);
    }
    return 0;
}

int
main(int argc, char **argv) {
    if (4 != argc) {
	printf("{\"name\":\"oj\",\"err\":\"expected 3 arguments\"}\n");
	exit(1);
    }
    const char	*mode = argv[1];
    const char	*filename = argv[2];
    long long	iter = strtoll(argv[3], NULL, 10);

    if (0 == strcmp(mode, "parse")) {
	bench_parse(filename, iter);
    }
    oj_cleanup();

    return 0;
}
