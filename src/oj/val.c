// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj.h"
#include "buf.h"
#include "intern.h"

bool oj_thread_safe = false;

typedef struct _Esc {
    int		len;
    const char	*seq;
} *Esc;

static struct _Esc	esc_map[0x20] = {
    { .len = 6, .seq = "\\u0000" },
    { .len = 6, .seq = "\\u0001" },
    { .len = 6, .seq = "\\u0002" },
    { .len = 6, .seq = "\\u0003" },
    { .len = 6, .seq = "\\u0004" },
    { .len = 6, .seq = "\\u0005" },
    { .len = 6, .seq = "\\u0006" },
    { .len = 6, .seq = "\\u0007" },
    { .len = 2, .seq = "\\b" },
    { .len = 2, .seq = "\\t" },
    { .len = 2, .seq = "\\n" },
    { .len = 6, .seq = "\\u000b" },
    { .len = 2, .seq = "\\f" },
    { .len = 2, .seq = "\\r" },
    { .len = 6, .seq = "\\u000e" },
    { .len = 6, .seq = "\\u000f" },
    { .len = 6, .seq = "\\u0010" },
    { .len = 6, .seq = "\\u0011" },
    { .len = 6, .seq = "\\u0012" },
    { .len = 6, .seq = "\\u0013" },
    { .len = 6, .seq = "\\u0014" },
    { .len = 6, .seq = "\\u0015" },
    { .len = 6, .seq = "\\u0016" },
    { .len = 6, .seq = "\\u0017" },
    { .len = 6, .seq = "\\u0018" },
    { .len = 6, .seq = "\\u0019" },
    { .len = 6, .seq = "\\u001a" },
    { .len = 6, .seq = "\\u001b" },
    { .len = 6, .seq = "\\u001c" },
    { .len = 6, .seq = "\\u001d" },
    { .len = 6, .seq = "\\u001e" },
    { .len = 6, .seq = "\\u001f" },
};

static struct _ojVal	*volatile free_head = NULL;
static struct _ojVal	*volatile free_tail = NULL;
static atomic_flag	val_busy = ATOMIC_FLAG_INIT;

static union ojS4k	*volatile s4k_head = NULL;
static union ojS4k	*volatile s4k_tail = NULL;
static atomic_flag	s4k_busy = ATOMIC_FLAG_INIT;

// TBD busy check for desctroy and frees

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
oj_cleanup() {
    union ojS4k	*s4k;
    while (NULL != (s4k = s4k_head)) {
	s4k_head = s4k->next;
	free(s4k);
    }
    s4k_tail = NULL;

    ojVal	val;
    while (NULL != (val = free_head)) {
	free_head = val->next;
	free(val);
    }
    free_tail = NULL;
}

void
oj_destroy(ojVal val) {
    ojVal	tail = val;
    ojVal	v = val;
    union ojS4k	*s4k_h = NULL;
    union ojS4k	*s4k_t = NULL;

    if (NULL == val) {
	return;
    }
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
	    if (sizeof(union ojS4k) < v->str.len) {
		free(v->str.ptr);
	    } else if (sizeof(v->str.raw) < v->str.len) {
		v->str.s4k->next = NULL;
		if (NULL == s4k_h) {
		    s4k_h = v->str.s4k;
		} else {
		    s4k_t->next = v->str.s4k;
		}
		s4k_t = v->str.s4k;
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
    if (NULL != s4k_h) {
	if (NULL == s4k_head) {
	    s4k_head = s4k_h;
	} else {
	    s4k_tail->next = s4k_h;
	}
	s4k_tail = s4k_t;
    }
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
    } else if (0 == stack->depth) {
	if (NULL != stack->cb) {
	    stack->cb(v, stack->ctx);
	} else {
	    stack->vals[stack->depth] = v;
	}
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

static void
buf_append_json(ojBuf buf, const char *s) {
    // TBD might be faster moving forward until a special char and then appending the string up till then
    for (; '\0' != *s; s++) {
	if ((byte)*s < 0x20) {
	    Esc	esc = esc_map + (byte)*s;
	    oj_buf_append_string(buf, esc->seq, esc->len);
	} else if ('"' == *s || '\\' == *s) {
	    oj_buf_append(buf, '\\');
	    oj_buf_append(buf, *s);
	} else {
	    oj_buf_append(buf, *s);
	}
    }
}

// TBD handle indent
size_t
oj_buf(ojBuf buf, ojVal val, int indent, int depth) {
    size_t	start = oj_buf_len(buf);

    if (NULL != val) {
	switch (val->type) {
	case OJ_NULL:
	    oj_buf_append_string(buf, "null", 4);
	    break;
	case OJ_TRUE:
	    oj_buf_append_string(buf, "true", 4);
	    break;
	case OJ_FALSE:
	    oj_buf_append_string(buf, "false", 5);
	    break;
	case OJ_INT:
	    // TBD if raw len is 0 then sprint
	    //  after that always use str
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
	case OJ_STRING: {
	    const char	*s;

	    if (sizeof(union ojS4k) < val->str.len) {
		s = val->str.ptr;
	    } else if (sizeof(val->str.raw) < val->str.len) {
		s = val->str.s4k->str;
	    } else {
		s = val->str.raw;
	    }
	    oj_buf_append(buf, '"');
	    buf_append_json(buf, s);
	    oj_buf_append(buf, '"');
	    break;
	}
	case OJ_OBJECT:
	    oj_buf_append(buf, '{');
	    if (OJ_OBJ_HASH == val->mod) {
		// TBD members
	    } else {
		int	d2 = depth + 1;

		for (ojVal v = val->list.head; NULL != v; v = v->next) {
		    // TBD handle longer key as well as with esc chars
		    oj_buf_append(buf, '"');
		    oj_buf_append_string(buf, v->key.raw, v->key.len);
		    oj_buf_append(buf, '"');
		    oj_buf_append(buf, ':');
		    oj_buf(buf, v, indent, d2);
		    if (NULL != v->next) {
			oj_buf_append(buf, ',');
		    }
		}
	    }
	    oj_buf_append(buf, '}');
	    break;
	case OJ_ARRAY: {
	    int	d2 = depth + 1;

	    oj_buf_append(buf, '[');
	    for (ojVal v = val->list.head; NULL != v; v = v->next) {
		oj_buf(buf, v, indent, d2);
		if (NULL != v->next) {
		    oj_buf_append(buf, ',');
		}
	    }
	    oj_buf_append(buf, ']');
	    break;
	}
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
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	val->str.s4k = s4k;
	memcpy(val->str.s4k->str, s, len);
	val->str.s4k->str[len] = '\0';
    } else {
	val->str.ptr = (char*)malloc(len + 1);
	val->str.cap = len + 1;
	memcpy(val->str.ptr, s, len);
	val->str.ptr[len] = '\0';
    }
    return OJ_OK;
}

const char*
oj_val_key(ojVal val) {
    const char	*k = NULL;

    if (NULL != val) {
	if (val->key.len < sizeof(val->key.raw)) {
	    k = val->key.raw;
	} else if (val->key.len < sizeof(union ojS4k)) {
	    k = val->key.s4k->str;
	} else {
	    k = val->key.ptr;
	}
    }
    return k;
}

const char*
oj_val_get_str(ojVal val) {
    const char	*s = NULL;

    if (NULL != val && OJ_STRING == val->type) {
	if (val->str.len < sizeof(val->str.raw)) {
	    s = val->str.raw;
	} else if (val->str.len < sizeof(union ojS4k)) {
	    s = val->str.s4k->str;
	} else {
	    s = val->str.ptr;
	}
    }
    return s;
}

int64_t
oj_val_get_int(ojVal val) {

    // TBD

    return 0;
}

long double
oj_val_get_float(ojVal val) {

    // TBD

    return 0.0;
}

const char*
oj_val_get_number(ojVal val) {

    // TBD

    return NULL;
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
	p->val.key.cap = len + 1;
	memcpy(p->val.key.ptr, s, len);
	p->val.key.ptr[len] = '\0';
    }
    p->val.key.len = len;
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
	if (NULL == p->val.str.ptr) {
	    OJ_ERR_MEM(&p->err, "string");
	    p->val.str.len = 0;
	    return;
	}
	memcpy(p->val.str.ptr, s, len);
	p->val.str.ptr[len] = '\0';
    }
    p->val.str.len = len;
}

void
_oj_append_str(ojParser p, ojStr str, const byte *s, size_t len) {
    size_t	nl = str->len + len;

    if (str->len < sizeof(str->raw)) {
	if (nl < sizeof(str->raw)) {
	    memcpy(str->raw + str->len, s, len);
	    str->raw[nl] = '\0';
	} else if (nl < sizeof(union ojS4k)) {
	    union ojS4k	*s4k = s4k_create();

	    memcpy(s4k->str, str->raw, str->len);
	    memcpy(s4k->str + str->len, s, len);
	    s4k->str[nl] = '\0';
	    str->s4k = s4k;
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)malloc(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(&p->err, "string");
		str->len = 0;
		return;
	    }
	    memcpy(ptr, str->raw, str->len);
	    memcpy(ptr + str->len, s, len);
	    ptr[nl] = '\0';
	    str->cap = cap;
	    str->ptr = ptr;
	}
    } else if (str->len < sizeof(union ojS4k)) {
	if (nl < sizeof(union ojS4k)) {
	    memcpy(str->s4k->str + str->len, s, len);
	    str->s4k->str[nl] = '\0';
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)malloc(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(&p->err, "string");
		str->len = 0;
		return;
	    }
	    memcpy(ptr, str->s4k->str, str->len);
	    memcpy(ptr + str->len, s, len);
	    ptr[nl] = '\0';
	    str->s4k->next = NULL;
	    if (NULL == s4k_head) {
		s4k_head = str->s4k;
	    } else {
		s4k_tail->next = str->s4k;
	    }
	    s4k_tail = str->s4k;
	    str->cap = cap;
	    str->ptr = ptr;
	}
    } else {
	if (nl < str->cap) {
	    memcpy(str->ptr + str->len, s, len);
	} else {
	    str->cap = nl * 3 / 2;
	    if (NULL == (str->ptr = realloc(str->ptr, str->cap))) {
		OJ_ERR_MEM(&p->err, "string");
		str->len = 0;
		return;
	    } else {
		memcpy(str->ptr + str->len, s, len);
	    }
	}
	str->ptr[nl] = '\0';
    }
    str->len = nl;
}

void
_oj_val_append_str(ojParser p, const byte *s, size_t len) {
    _oj_append_str(p, &p->val.str, s, len);
}


// TBD append to key also
