// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oj/oj.h"
#include "../helper.h"

typedef struct _mode {
    const char	*key;
    void	(*func)(const char *filename, long long iter);
} *Mode;

static void
form_result(long long iter, long long usec, ojErr err) {
    if (OJ_OK == err->code) {
	form_json_results("oj", iter, usec, NULL);
    } else {
	char	msg[256];

	snprintf(msg, sizeof(msg), "%s at %d:%d", err->msg, err->line, err->col);
	form_json_results("oj", iter, usec, msg);
    }
}

// Single file parsing after loading into memory.
static void
validate(const char *filename, long long iter) {
    int64_t		dt;
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();
    struct _ojErr	err = OJ_ERR_INIT;

    for (int i = iter; 0 < i; i--) {
	oj_validate_str(&err, buf);
    }
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
    if (NULL != buf) {
	free(buf);
    }
}

static void
parse(const char *filename, long long iter) {
    int64_t		dt;
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();
    struct _ojReuser	r;
    struct _ojErr	err = OJ_ERR_INIT;

    for (int i = iter; 0 < i; i--) {
	oj_parse_str(&err, buf, &r);
	oj_reuse(&r);
    }
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
    if (NULL != buf) {
	free(buf);
    }
}

static ojCallbackOp
one_cb(ojVal val, void *ctx) {
    ojVal	v = oj_object_find(val, "timestamp", 9);

    if (NULL != v && OJ_INT == v->type && v->num.fixnum < 1000000LL) {
	printf("--- timestamp out of bounds ---\n");
    }
    *(long long*)ctx = *(long long*)ctx + 1;

    return OJ_DESTROY;
}

static void
parse_one(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;

    iter = 0;

    int64_t		start = clock_micro();

    oj_parse_file_cb(&err, filename, one_cb, &iter);
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
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
		cnt += walk(v);
	    }
	} else {
	    ojVal	*b = val->hash;
	    ojVal	*bend = b + (sizeof(val->hash) / sizeof(*val->hash));
	    ojVal	v;

	    for (; b < bend; b++) {
		for (v = *b; NULL != v; v = v->next) {
		    cnt += walk(v);
		}
	    }
	}
	break;
    case OJ_ARRAY:
	for (ojVal v = val->list.head; NULL != v; v = v->next) {
	    cnt += walk(v);
	}
	break;
    }
    return cnt;
}

static ojCallbackOp
each_cb(ojVal val, void *ctx) {
    *(long long*)ctx = *(long long*)ctx + 1;

    if (walk(val) < 0) {
	return OJ_CONTINUE;
    }
    return OJ_DESTROY;
}

static void
parse_each(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;

    iter = 0;

    int64_t	start = clock_micro();

    oj_parse_file_cb(&err, filename, each_cb, &iter);
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
}

static ojCallbackOp
heavy_cb(ojVal val, void *ctx) {
    int		delay = walk(val) / 100 + 5;
    int64_t	done = clock_micro() + delay;

    while (clock_micro() < done) {
	continue;
    }
    *(long long*)ctx = *(long long*)ctx + 1;

    return OJ_DESTROY;
}

static void
parse_heavy(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _ojCaller	caller;

    oj_thread_safe = true;
    iter = 0;

    oj_caller_start(&err, &caller, heavy_cb, &iter);

    int64_t	start = clock_micro();

    oj_parse_file_call(&err, filename, &caller);
    oj_caller_wait(&caller);
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
}

static void
test(const char *filename, long long iter) {
    ojVal		v;
    char		*buf = load_file(filename);
    struct _ojErr	err = OJ_ERR_INIT;

    v = oj_parse_str(&err, buf, NULL);
    oj_destroy(v);
    form_result(1, 0, &err);
    if (NULL != buf) {
	free(buf);
    }
}

//static const char	*modes[] = { "validate", "parse", "multiple-one", "multiple-each", "multiple-heavy", "encode", "test", NULL };

static struct _mode	mode_map[] = {
    { .key = "validate", .func = validate },
    { .key = "parse", .func = parse },
    { .key = "multiple-one", .func = parse_one },
    { .key = "multiple-each", .func = parse_each },
    { .key = "multiple-heavy", .func = parse_heavy },
    { .key = "test", .func = test },
    { .key = NULL },
};

int
main(int argc, char **argv) {
    if (4 != argc) {
	printf("{\"name\":\"oj\",\"err\":\"expected 3 arguments\"}\n");
	exit(1);
    }
    const char	*mode = argv[1];
    const char	*filename = argv[2];
    long long	iter = strtoll(argv[3], NULL, 10);

    for (Mode m = mode_map; NULL != m->key; m++) {
	if (0 == strcmp(mode, m->key)) {
	    m->func(filename, iter);
	}
    }
    oj_cleanup();

    return 0;
}
