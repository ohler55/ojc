// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj.h"
#include "buf.h"

bool oj_thread_safe = false;

struct _ojVal		*volatile free_head = NULL;
struct _ojVal		*volatile free_tail = NULL;
static atomic_flag	val_busy = ATOMIC_FLAG_INIT;

union ojS4k		*volatile s4k_head = NULL;
union ojS4k		*volatile s4k_tail = NULL;
static atomic_flag	s4k_busy = ATOMIC_FLAG_INIT;

ojVal
oj_val_create() {
    ojVal	val = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == free_head) {
	val = (ojVal)calloc(1, sizeof(struct _ojVal));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	if (oj_thread_safe) {
	    while (atomic_flag_test_and_set(&val_busy)) {
	    }
	}
	if (NULL == free_head) {
	    val = (ojVal)calloc(1, sizeof(struct _ojVal));
	} else {
	    val = free_head;
	    free_head = free_head->next;
	}
	if (oj_thread_safe) {
	    atomic_flag_clear(&val_busy);
	}
    }
    return val;
}

union ojS4k*
s4k_create() {
    union ojS4k	*s = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == s4k_head) {
	s = (union ojS4k*)calloc(1, sizeof(union ojS4k));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	if (oj_thread_safe) {
	    while (atomic_flag_test_and_set(&s4k_busy)) {
	    }
	}
	if (NULL == s4k_head) {
	    s = (union ojS4k*)calloc(1, sizeof(union ojS4k));
	} else {
	    s = s4k_head;
	    s4k_head = s4k_head->next;
	}
	if (oj_thread_safe) {
	    atomic_flag_clear(&s4k_busy);
	}
    }
    return s;
}

void
oj_destroy(ojVal val) {
    ojVal	tail = val;
    ojVal	v = val;
    union ojS4k	*s4k_h = NULL;
    union ojS4k	*s4k_t = NULL;

    val->next = NULL;
    for (; NULL != v; v = v->next) {
	if (sizeof(union ojS4k) < v->key.len) {
	    free(v->key.ptr);
	} else if (sizeof(v->key.raw) < v->key.len) {
	    v->key.s4k->next = NULL;
	    if (NULL == s4k_h) {
		s4k_h = v->key.s4k;
	    } else {
		s4k_t->next = v->key.s4k;
	    }
	    s4k_t = v->key.s4k;
	}
	switch (v->type) {
	case OJ_STRING:
	    switch (v->mod) {
	    case OJ_STR_4K:
		v->str.s4k->next = NULL;
		if (NULL == s4k_h) {
		    s4k_h = v->str.s4k;
		} else {
		    s4k_t->next = v->str.s4k;
		}
		s4k_t = v->str.s4k;
		break;
	    case OJ_STR_PTR:
		free(v->str.ptr);
		break;
	    }
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
    if (NULL == free_head) {
	free_head = val;
    } else {
	free_tail->next = val;
    }
    free_tail = tail;
    if (NULL == s4k_head) {
	s4k_head = s4k_h;
    } else {
	s4k_tail->next = s4k_h;
    }
    s4k_tail = s4k_t;
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
	    switch (val->mod) {
	    case OJ_STR_INLINE:
		oj_buf_append_string(buf, val->str.raw, (size_t)val->str.len);
		break;
	    case OJ_STR_4K:
		oj_buf_append_string(buf, val->str.s4k->str, (size_t)val->str.len);
		break;
	    case OJ_STR_PTR:
		oj_buf_append_string(buf, val->str.ptr, (size_t)val->str.len);
		break;
	    }
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

ojStatus
oj_val_set_str(ojErr err, ojVal val, const char *s, size_t len) {
    if (len < sizeof(val->str.raw)) {
	memcpy(val->str.raw, s, len);
	val->str.raw[len] = '\0';
	val->mod = OJ_STR_INLINE;
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	val->str.s4k = s4k;
	memcpy(val->str.s4k->str, s, len);
	val->str.s4k->str[len] = '\0';
	val->mod = OJ_STR_4K;
    } else {
	val->str.ptr = (char*)malloc(len + 1);
	memcpy(val->str.ptr, s, len);
	val->str.ptr[len] = '\0';
	val->mod = OJ_STR_PTR;
    }
    return OJ_OK;
}

void
_oj_val_set_key(ojParser p, const char *s, size_t len) {
    if (len < sizeof(p->val.key.raw)) {
	memcpy(p->val.key.raw, s, len);
	p->val.key.raw[len] = '\0';
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	p->val.key.s4k = s4k;
	memcpy(p->val.key.s4k->str, s, len);
	p->val.key.s4k->str[len] = '\0';
    } else {
	p->val.key.ptr = (char*)malloc(len + 1);
	memcpy(p->val.key.ptr, s, len);
	p->val.key.ptr[len] = '\0';
    }
}

void
_oj_val_set_str(ojParser p, const char *s, size_t len) {
    if (len < sizeof(p->val.str.raw)) {
	memcpy(p->val.str.raw, s, len);
	p->val.str.raw[len] = '\0';
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	p->val.str.s4k = s4k;
	memcpy(p->val.str.s4k->str, s, len);
	p->val.str.s4k->str[len] = '\0';
    } else {
	p->val.str.ptr = (char*)malloc(len + 1);
	memcpy(p->val.str.ptr, s, len);
	p->val.str.ptr[len] = '\0';
    }
}
