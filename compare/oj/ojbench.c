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
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();
    char		mem[16];
    struct _ojReuser	r;
    struct _ojErr	err = OJ_ERR_INIT;;

    for (int i = iter; 0 < i; i--) {
	v = oj_parse_strd(&err, buf, &r);
	oj_reuse(&r);
    }
    dt = clock_micro() - start;

    mem_use(mem, sizeof(mem));
    if (OJ_OK == err.code) {
	form_json_results("oj", iter, dt, mem, NULL);
    } else {
	char	msg[256];

	snprintf(msg, sizeof(msg), "%s at %d:%d", err.msg, err.line, err.col);
	form_json_results("oj", iter, dt, mem, msg);
    }
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
