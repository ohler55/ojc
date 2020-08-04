// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj.h"
#include "buf.h"

#define USE_MUTEX 0
#define USE_ATOMIC 1

static struct _ojList		free_vals = { .head = NULL, .tail = NULL };
//static struct _ojExtList	free_exts = { .head = NULL, .tail = NULL };
static atomic_flag		val_busy = ATOMIC_FLAG_INIT;
//static atomic_flag		ext_busy = ATOMIC_FLAG_INIT;

bool oj_thread_safe = false;

ojVal
oj_val_create() {
    ojVal	val = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == free_vals.head) {
	val = (ojVal)calloc(1, sizeof(struct _ojVal));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	if (oj_thread_safe) {
	    while (atomic_flag_test_and_set(&val_busy)) {
	    }
	}
	if (NULL == free_vals.head) {
	    val = (ojVal)calloc(1, sizeof(struct _ojVal));
	} else {
	    val = free_vals.head;
	    free_vals.head = free_vals.head->next;
	}
	if (oj_thread_safe) {
	    atomic_flag_clear(&val_busy);
	}
    }
    return val;
}

void
oj_destroy(ojVal val) {
    ojVal	tail = val;
    ojVal	v = val;

    val->next = NULL;
    for (; NULL != v; v = v->next) {
	switch (v->type) {
	case OJ_STRING:
	    break;
	case OJ_INT:
	    // TBD if raw and ext then free the exts
	    break;
	case OJ_DECIMAL:
	    // TBD if raw and ext then free the exts
	    break;
	case OJ_OBJECT:
	    if (OJ_OBJ_RAW == v->mod) {
		tail->next = v->list.head;
		tail = v->list.tail;
	    } else {
		printf("*********** hash\n");
		// TBD each hash bucket
	    }
	    break;
	case OJ_ARRAY: {
	    tail->next = v->list.head;
	    tail = v->list.tail;
	    break;
	}
	default:
	    break;
	}
	v->type = OJ_FREE;
    }
    if (oj_thread_safe) {
	while (atomic_flag_test_and_set(&val_busy)) {
	}
    }
    if (NULL == free_vals.head) {
	free_vals.head = val;
    } else {
	free_vals.tail->next = val;
    }
    free_vals.tail = tail;
    if (oj_thread_safe) {
	atomic_flag_clear(&val_busy);
    }
}

typedef struct _Stack {
    // TBD change to be expandable
    ojVal		vals[256];
    int			depth;
    ojParseCallback	cb;
    void		*ctx;
} *Stack;

static void
push(ojVal val, void *ctx) {
    Stack	stack = (Stack)ctx;
    ojVal	v = oj_val_create();

    *v = *val;
    if (0 < stack->depth) {
	ojVal	p = stack->vals[stack->depth - 1];

	if (NULL == p->list.head) {
	    p->list.head = v;
	} else {
	    p->list.tail->next = v;
	}
	p->list.tail = v;
    }
    if (OJ_OBJECT == v->type || OJ_ARRAY == v->type) {
	stack->vals[stack->depth] = v;
	stack->depth++;
    } else if (0 == stack->depth && NULL != stack->cb) {
	stack->cb(v, stack->ctx);
    }
}

static void
pop(void *ctx) {
    Stack	stack = (Stack)ctx;

    stack->depth--;
    if (0 == stack->depth && NULL != stack->cb) {
	stack->cb(*stack->vals, stack->ctx);
	*stack->vals = NULL;
    }
}

void
oj_val_parser(ojParser p) {
    // TBD is this needed?
    p->push = push;
    p->pop = pop;
    //p->pp_ctx = &stack;
    oj_err_init(&p->err);
}

ojVal
oj_val_parse_str(ojErr err, const char *json, ojParseCallback cb, void *ctx) {
    struct _ojParser	p;
    struct _Stack	stack = {
	.vals = { NULL },
	.depth = 0,
	.cb = cb,
	.ctx = ctx,
    };

    memset(&p, 0, sizeof(p));
    p.push = push;
    p.pop = pop;
    p.ctx = &stack;

    oj_parse_str(&p, json);
    if (OJ_OK != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return stack.vals[0];
}

ojVal
oj_val_parse_strp(ojErr err, const char **json) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_file(ojErr err, const char *filename, ojParseCallback cb, void *ctx) {
    struct _ojParser	p;
    struct _Stack	stack = {
	.vals = { NULL },
	.depth = 0,
	.cb = cb,
	.ctx = ctx,
    };

    memset(&p, 0, sizeof(p));
    p.push = push;
    p.pop = pop;
    p.ctx = &stack;

    oj_parse_file(&p, filename);
    if (OJ_OK != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return stack.vals[0];
}

ojVal
oj_val_parse_fd(ojErr err, int fd, ojParseCallback cb, void *ctx) {
    struct _ojParser	p;
    struct _Stack	stack = {
	.vals = { NULL },
	.depth = 0,
	.cb = cb,
	.ctx = ctx,
    };

    memset(&p, 0, sizeof(p));
    p.push = push;
    p.pop = pop;
    p.ctx = &stack;

    oj_parse_fd(&p, fd);
    if (OJ_OK != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return stack.vals[0];
}

ojVal
oj_val_parse_reader(ojErr err, void *src, ojReadFunc rf) {

    // TBD

    return NULL;
}

size_t
oj_buf(ojBuf buf, ojVal val, int indent, int depth) {
    size_t	start = oj_buf_len(buf);

    // TBD handle nested
    if (NULL != val) {
	switch (val->type) {
	case OJ_NULL:
	    oj_buf_append_string(buf, "null", 4);
	    break;
	case OJ_TRUE:
	    oj_buf_append_string(buf, "true", 4);
	    break;
	case OJ_FALSE:
	    oj_buf_append_string(buf, "false", 4);
	    break;
	case OJ_INT:
	    if (OJ_INT_RAW == val->mod) {
		oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    } else {
		// TBD
	    }
	    break;
	case OJ_DECIMAL:
	    if (OJ_DEC_RAW == val->mod) {
		oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    } else {
		// TBD
	    }
	    break;
	case OJ_STRING:
	    oj_buf_append(buf, '"');
	    // TBD handle extended
	    oj_buf_append_string(buf, val->str.start, (size_t)val->str.len);
	    oj_buf_append(buf, '"');
	    break;
	case OJ_OBJECT:
	    oj_buf_append(buf, '{');
	    // TBD members
	    oj_buf_append(buf, '}');
	    break;
	case OJ_ARRAY:
	    oj_buf_append(buf, '[');
	    // TBD members
	    oj_buf_append(buf, ']');
	    break;
	default:
	    oj_buf_append_string(buf, "??", 2);
	    break;
	}
    }
    return oj_buf_len(buf) - start;
}
