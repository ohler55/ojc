// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
	char	msg[300];

	snprintf(msg, sizeof(msg), "%s at %d:%d", err->msg, err->line, err->col);
	form_json_results("oj", iter, usec, msg);
    }
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

// Single file parsing after loading into memory.
static void
validate(const char *filename, long long iter) {
    int64_t		dt;
    char		*buf = load_file(filename);
    struct _ojErr	err = OJ_ERR_INIT;
    int64_t		start = clock_micro();

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

typedef struct _cnt {
    long long	iter;
    int		depth;
} *Cnt;

static void
push_light(ojVal val, void *ctx) {
    switch (val->type) {
    case OJ_OBJECT:
    case OJ_ARRAY:
	((Cnt)ctx)->depth++;
	break;
    }
}

static void
pop_light(void *ctx) {
    Cnt	c = (Cnt)ctx;

    c->depth--;
    if (c->depth <= 0) {
	c->iter++;
    }
}

static void
parse_light(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _cnt		c = { .depth = 0, .iter = 0 };

    int64_t		start = clock_micro();

    oj_pp_parse_file(&err, filename, push_light, pop_light, &c);
    dt = clock_micro() - start;
    form_result(c.iter, dt, &err);
}

static ojCallbackOp
heavy_cb(ojVal val, void *ctx) {
    int64_t	done = clock_micro() + 8;

    while (clock_micro() < done) {
	continue;
    }
    walk(val);
    *(long long*)ctx = *(long long*)ctx + 1;

    return OJ_DESTROY;
}

static void
parse_heavy(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _ojCaller	caller;

    oj_caller_start(&err, &caller, heavy_cb, &iter);
    oj_thread_safe = true;
    iter = 0;

    int64_t	start = clock_micro();

    oj_parse_file_call(&err, filename, &caller);
    oj_caller_wait(&caller);
    dt = clock_micro() - start;
    form_result(iter, dt, &err);
}

static void
test(const char *filename, long long iter) {
    char		*buf = load_file(filename);
    struct _ojErr	err = OJ_ERR_INIT;

    oj_validate_str(&err, buf);
    // This also works:
    //oj_destroy(oj_parse_str(&err, buf, NULL));
    form_result(1, 0, &err);
    if (NULL != buf) {
	free(buf);
    }
}

static struct _mode	mode_map[] = {
    { .key = "validate", .func = validate },
    { .key = "parse", .func = parse },
    { .key = "multiple-light", .func = parse_light },
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
