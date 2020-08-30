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
	form_json_results("oj-pp", iter, usec, NULL);
    } else {
	char	msg[256];

	snprintf(msg, sizeof(msg), "%s at %d:%d", err->msg, err->line, err->col);
	form_json_results("oj-pp", iter, usec, msg);
    }
}

static void
push_noop(ojVal val, void *ctx) {
}

static void
pop_noop(void *ctx) {
}

static void
parse(const char *filename, long long iter) {
    int64_t		dt;
    char		*buf = load_file(filename);
    int64_t		start = clock_micro();
    struct _ojErr	err = OJ_ERR_INIT;

    for (int i = iter; 0 < i; i--) {
	oj_pp_parse_str(&err, buf, push_noop, pop_noop, NULL);
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
    int		cnt;
} *Cnt;

static void
push_one(ojVal val, void *ctx) {
    switch (val->type) {
    case OJ_OBJECT:
    case OJ_ARRAY:
	((Cnt)ctx)->depth++;
	break;
    case OJ_INT:
	if (9 == val->key.len && 0 == strcmp("timestamp", oj_key(val)) && val->num.fixnum < 1000000LL) {
	    printf("--- timestamp out of bounds ---\n");
	}
	break;
    }
}

static void
pop_one(void *ctx) {
    Cnt	c = (Cnt)ctx;

    c->depth--;
    if (c->depth <= 0) {
	c->iter++;
    }
}

static void
parse_one(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _cnt		c = { .depth = 0, .iter = 0, .cnt = 0 };

    int64_t		start = clock_micro();

    oj_pp_parse_file(&err, filename, push_one, pop_one, &c);
    dt = clock_micro() - start;
    form_result(c.iter, dt, &err);
}

static void
push_each(ojVal val, void *ctx) {
    Cnt	c = (Cnt)ctx;

    switch (val->type) {
    case OJ_TRUE:
	break;
    case OJ_NULL:
    case OJ_FALSE:
	c->cnt++;
	break;
    case OJ_INT:
	if (0 == val->num.fixnum) {
	    c->cnt++;
	}
	break;
    case OJ_DECIMAL:
	if (0.0 == val->num.dub) {
	    c->cnt++;
	}
	break;
    case OJ_STRING:
	if ('\0' == *val->str.raw) {
	    c->cnt++;
	}
	break;
    case OJ_OBJECT:
    case OJ_ARRAY:
	((Cnt)ctx)->depth++;
	break;
    }
}

static void
pop_each(void *ctx) {
    Cnt	c = (Cnt)ctx;

    c->depth--;
    if (c->depth <= 0) {
	c->iter++;
    }
}

static void
parse_each(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _cnt		c = { .depth = 0, .iter = 0, .cnt = 0 };

    int64_t		start = clock_micro();

    oj_pp_parse_file(&err, filename, push_each, pop_each, &c);
    dt = clock_micro() - start;
    form_result(c.iter, dt, &err);
}


static void
push_heavy(ojVal val, void *ctx) {
    Cnt	c = (Cnt)ctx;

    switch (val->type) {
    case OJ_TRUE:
	break;
    case OJ_NULL:
    case OJ_FALSE:
	c->cnt++;
	break;
    case OJ_INT:
	if (0 == val->num.fixnum) {
	    c->cnt++;
	}
	break;
    case OJ_DECIMAL:
	if (0.0 == val->num.dub) {
	    c->cnt++;
	}
	break;
    case OJ_STRING:
	if ('\0' == *val->str.raw) {
	    c->cnt++;
	}
	break;
    case OJ_OBJECT:
    case OJ_ARRAY:
	((Cnt)ctx)->depth++;
	break;
    }
}

static void
pop_heavy(void *ctx) {
    Cnt	c = (Cnt)ctx;

    c->depth--;
    if (c->depth <= 0) {
	int64_t	done = clock_micro() + 5;

	c->iter++;
	while (clock_micro() < done) {
	    continue;
	}
    }
}

static void
parse_heavy(const char *filename, long long iter) {
    int64_t		dt;
    struct _ojErr	err = OJ_ERR_INIT;
    struct _cnt		c = { .depth = 0, .iter = 0, .cnt = 0 };

    int64_t		start = clock_micro();

    oj_pp_parse_file(&err, filename, push_heavy, pop_heavy, &c);
    dt = clock_micro() - start;
    form_result(c.iter, dt, &err);
}

static void
test(const char *filename, long long iter) {
    char		*buf = load_file(filename);
    struct _ojErr	err = OJ_ERR_INIT;

    oj_pp_parse_str(&err, buf, push_noop, pop_noop, NULL);
    form_result(1, 0, &err);
    if (NULL != buf) {
	free(buf);
    }
}

//static const char	*modes[] = { "validate", "parse", "multiple-one", "multiple-each", "multiple-heavy", "encode", "test", NULL };

static struct _mode	mode_map[] = {
    { .key = "validate", .func = parse },
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
