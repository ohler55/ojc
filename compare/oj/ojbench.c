// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oj/oj.h"
#include "../helper.h"

static void
bench_parse(const char *filename, long long iter) {
    int64_t		dt;
    ojVal		v;
    struct _ojParser	p;
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();
    char		mem[16];

    oj_val_parser_init(&p);
    for (int i = iter; 0 < i; i--) {
	v = oj_val_parse_str(&p, buf, NULL, NULL);
	oj_destroy(v);
    }
    dt = clock_micro() - start;

    form_json_results("oj", iter, dt, mem_use(mem, sizeof(mem)), OJ_OK == p.err.code ? NULL : p.err.msg);

    if (NULL != buf) {
	free(buf);
    }
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
