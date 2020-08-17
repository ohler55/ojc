// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj.h"
#include "buf.h"
#include "debug.h"
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

static uint32_t
calc_hash(const char *key, size_t len) {
    uint32_t	h = 0;
    const byte	*k = (const byte*)key;
    const byte	*kend = k + len;

    for (; k < kend; k++) {
	// fast, just spread it out
	h = 77 * h + (*k - 0x2D);
    }
    return h;
}

ojVal
oj_val_create() {
    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == free_head) {
	return (ojVal)OJ_CALLOC(1, sizeof(struct _ojVal));
    }
    if (!oj_thread_safe) {
	ojVal	val = free_head;

	free_head = free_head->free;

	return val;
    }
    ojVal	val;

    while (atomic_flag_test_and_set(&val_busy)) {
    }
    if (NULL == free_head) {
	val = (ojVal)OJ_CALLOC(1, sizeof(struct _ojVal));
    } else {
	val = free_head;
	free_head = free_head->free;
    }
    atomic_flag_clear(&val_busy);

    return val;
}

union ojS4k*
s4k_create() {
    union ojS4k	*s = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == s4k_head) {
	s = (union ojS4k*)OJ_CALLOC(1, sizeof(union ojS4k));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	if (oj_thread_safe) {
	    while (atomic_flag_test_and_set(&s4k_busy)) {
	    }
	}
	if (NULL == s4k_head) {
	    s = (union ojS4k*)OJ_CALLOC(1, sizeof(union ojS4k));
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
	OJ_FREE(s4k);
    }
    s4k_tail = NULL;

    ojVal	val;
    while (NULL != (val = free_head)) {
	free_head = val->free;
	OJ_FREE(val);
    }
    free_tail = NULL;
}

void
_oj_val_clear(ojVal v) {
    union ojS4k	*s4k_h = NULL;
    union ojS4k	*s4k_t = NULL;

    if (sizeof(union ojS4k) < v->key.len) {
	OJ_FREE(v->key.ptr);
    } else if (sizeof(v->key.raw) < v->key.len) {
	v->key.s4k->next = NULL;
	if (NULL == s4k_h) {
	    s4k_h = v->key.s4k;
	} else {
	    s4k_t->next = v->key.s4k;
	}
	s4k_t = v->key.s4k;
    }
    v->key.len = 0;
    switch (v->type) {
    case OJ_STRING:
	if (sizeof(union ojS4k) < v->str.len) {
	    OJ_FREE(v->str.ptr);
	} else if (sizeof(v->str.raw) < v->str.len) {
	    v->str.s4k->next = NULL;
	    if (NULL == s4k_h) {
		s4k_h = v->str.s4k;
	    } else {
		s4k_t->next = v->str.s4k;
	    }
	    s4k_t = v->str.s4k;
	}
	v->str.len = 0;
	break;
    case OJ_BIG:
	if (sizeof(v->num.raw) <= v->num.len) {
	    OJ_FREE(v->num.ptr);
	}
	v->key.len = 0;
	break;
    }
    if (oj_thread_safe) {
	while (atomic_flag_test_and_set(&val_busy)) {
	}
	if (NULL != s4k_h) {
	    if (NULL == s4k_head) {
		s4k_head = s4k_h;
	    } else {
		s4k_tail->next = s4k_h;
	    }
	    s4k_tail = s4k_t;
	}
	atomic_flag_clear(&val_busy);
    } else {
	if (NULL != s4k_h) {
	    if (NULL == s4k_head) {
		s4k_head = s4k_h;
	    } else {
		s4k_tail->next = s4k_h;
	    }
	    s4k_tail = s4k_t;
	}
    }
}

void
oj_reuse(ojReuser reuser) {
    ojVal	v;
    ojVal	next;
    union ojS4k	*s4k_h = NULL;
    union ojS4k	*s4k_t = NULL;

    // TBD walk dig, destroy str and big and place on head
    // free up
    for (v = reuser->dig; NULL != v; v = next) {
	next = v->free;
	if (sizeof(union ojS4k) < v->key.len) {
	    OJ_FREE(v->key.ptr);
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
		OJ_FREE(v->str.ptr);
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
	case OJ_BIG:
	    if (sizeof(v->num.raw) <= v->num.len) {
		OJ_FREE(v->num.ptr);
	    }
	    break;
	}
	v->free = reuser->head;
	reuser->head = v;
    }
    if (oj_thread_safe) {
	while (atomic_flag_test_and_set(&val_busy)) {
	}
	if (NULL == free_head) {
	    free_head = reuser->head;
	} else {
	    free_tail->free = reuser->head;
	}
	free_tail = reuser->tail;
	if (NULL != s4k_h) {
	    if (NULL == s4k_head) {
		s4k_head = s4k_h;
	    } else {
		s4k_tail->next = s4k_h;
	    }
	    s4k_tail = s4k_t;
	}
	atomic_flag_clear(&val_busy);
    } else {
	if (NULL == free_head) {
	    free_head = reuser->head;
	} else {
	    free_tail->free = reuser->head;
	}
	free_tail = reuser->tail;
	if (NULL != s4k_h) {
	    if (NULL == s4k_head) {
		s4k_head = s4k_h;
	    } else {
		s4k_tail->next = s4k_h;
	    }
	    s4k_tail = s4k_t;
	}
    }
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
    val->free = NULL;
    for (; NULL != v; v = v->free) {
	if (sizeof(union ojS4k) < v->key.len) {
	    OJ_FREE(v->key.ptr);
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
		OJ_FREE(v->str.ptr);
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
	case OJ_DECIMAL:
	    break;
	case OJ_BIG:
	    if (sizeof(v->num.raw) <= v->num.len) {
		OJ_FREE(v->num.ptr);
	    }
	    break;
	case OJ_OBJECT:
	    if (OJ_OBJ_RAW == v->mod) {
		for (ojVal m = v->list.head; NULL != m; m = m->next) {
		    m->free = NULL;
		    tail->free = m;
		    tail = m;
		}
	    } else {
		ojVal	*bucket = v->hash;
		ojVal	*bend = bucket + sizeof(v->hash) / sizeof(*v->hash);

		for (; bucket < bend; bucket++) {
		    for (ojVal m = *bucket; NULL != m; m = m->next) {
			m->free = NULL;
			tail->free = m;
			tail = m;
		    }
		}
	    }
	    break;
	case OJ_ARRAY: {
	    for (ojVal m = v->list.head; NULL != m; m = m->next) {
		m->free = NULL;
		tail->free = m;
		tail = m;
	    }
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
	if (NULL == free_head) {
	    free_head = val;
	} else {
	    free_tail->free = val;
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
	atomic_flag_clear(&val_busy);
    } else {
	if (NULL == free_head) {
	    free_head = val;
	} else {
	    free_tail->free = val;
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
    }
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
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%lld", (long long)val->num.fixnum);
	    }
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    break;
	case OJ_DECIMAL:
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%Lg", val->num.dub);
	    }
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    break;
	case OJ_BIG:
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
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
		ojVal	*bucket = val->hash;
		ojVal	*bend = bucket + sizeof(val->hash) / sizeof(*val->hash);
		ojVal	v;
		int	d2 = depth + 1;
		bool	first = true;

		oj_buf_append(buf, '{');
		for (; bucket < bend; bucket++) {
		    for (v = *bucket; NULL != v; v = v->next) {
			if (first) {
			    first = false;
			} else {
			    oj_buf_append(buf, ',');
			}
			oj_buf_append(buf, '"');
			oj_buf_append_string(buf, v->key.raw, v->key.len);
			oj_buf_append(buf, '"');
			oj_buf_append(buf, ':');
			oj_buf(buf, v, indent, d2);
		    }
		}
		oj_buf_append(buf, '}');
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

char*
oj_to_str(ojVal val, int indent) {
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    oj_buf(&buf, val, 0, 0);
    oj_buf_append(&buf, '\0');
    if (buf.base == buf.head) {
	return strdup(buf.head);
    }
    return buf.head;
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
	val->str.ptr = (char*)OJ_MALLOC(len + 1);
	val->str.cap = len + 1;
	memcpy(val->str.ptr, s, len);
	val->str.ptr[len] = '\0';
    }
    return OJ_OK;
}

const char*
oj_key(ojVal val) {
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
oj_str_get(ojVal val) {
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
oj_int_get(ojVal val) {
    int64_t	i = 0;

    if (NULL != val && OJ_INT == val->type) {
	i = val->num.fixnum;
    }
    return i;
}

long double
oj_double_get(ojVal val, bool prec) {
    long double	d = 0.0;

    if (NULL != val && OJ_DECIMAL == val->type) {
	d = val->num.dub;
    }
    return d;
}

const char*
oj_bignum_get(ojVal val) {
    const char	*s = NULL;

    if (NULL != val) {
	switch (val->type) {
	case OJ_INT:
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%lld", (long long)val->num.fixnum);
	    }
	    s = val->num.raw;
	    break;
	case OJ_DECIMAL:
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%Lg", val->num.dub);
	    }
	    s = val->num.raw;
	    break;
	case OJ_BIG:
	    if (sizeof(val->num.raw) <= val->num.len) {
		s = val->num.ptr;
	    } else {
		s = val->num.raw;
	    }
	}
    }
    return s;
}

ojVal
oj_array_first(ojVal val) {
    ojVal	v = NULL;

    if (NULL != val && OJ_ARRAY == val->type) {
	v = val->list.head;
    }
    return v;
}

ojVal
oj_array_last(ojVal val) {
    ojVal	v = NULL;

    if (NULL != val && OJ_ARRAY == val->type) {
	v = val->list.tail;
    }
    return v;
}

ojVal
oj_array_nth(ojVal val, int n) {
    ojVal	v = NULL;

    if (NULL != val && OJ_ARRAY == val->type) {
	for (v = val->list.head; NULL != v && 0 < n; n--, v = v->next) {
	}
    }
    return v;
}

ojVal
oj_each(ojVal val, bool (*cb)(ojVal v, void* ctx), void *ctx) {
    ojVal	v = NULL;

    if (NULL != val) {
	switch (val->type) {
	case OJ_ARRAY:
	    for (v = val->list.head; NULL != v; v = v->next) {
		if (!cb(v, ctx)) {
		    break;
		}
	    }
	    break;
	case OJ_OBJECT:
	    if (OJ_OBJ_RAW == val->mod) {
		for (v = val->list.head; NULL != v; v = v->next) {
		    if (!cb(v, ctx)) {
			break;
		    }
		}
	    } else {
		ojVal	*bucket = val->hash;
		ojVal	*bend = bucket + sizeof(val->hash) / sizeof(*val->hash);

		for (; bucket < bend; bucket++) {
		    for (v = *bucket; NULL != v; v = v->next) {
			if (!cb(v, ctx)) {
			    break;
			}
		    }
		    if (NULL != v) {
			break;
		    }
		}
	    }
	    break;
	}
    }
    return v;
}

ojVal
oj_object_get(ojVal val, const char *key, int len) {
    ojVal	v = NULL;

    if (NULL != val && OJ_OBJECT == val->type) {
	if (OJ_OBJ_RAW == val->mod) {
	    v = val->list.head;
	    // memset(val->hash, 0, sizeof(val->hash));
	    for (; NULL != v; v = v->next) {
		v->kh = calc_hash(oj_key(v), val->key.len);
		// place into hash
	    }
	    // TBD val->type = OJ_OBJ_HASH;

	    // TBD until then...
	    for (v = val->list.head; NULL != v; v = v->next) {
		if (len == v->key.len && 0 == strcmp(key, oj_key(v))) {
		    break;
		}
	    }
	}
    }
    return v;
}

ojVal
oj_object_find(ojVal val, const char *key, int len) {
    ojVal	v = NULL;

    if (NULL != val && OJ_OBJECT == val->type) {
	if (OJ_OBJ_RAW == val->mod) {
	    for (v = val->list.head; NULL != v; v = v->next) {
		if (len == v->key.len && 0 == strncmp(key, oj_key(v), len)) {
		    break;
		}
	    }
	} else {
	    // TBD
	}
    }
    return v;
}

void
_oj_val_set_key(ojVal val, const char *s, size_t len) {
    if (len < sizeof(val->key.raw)) {
	memcpy(val->key.raw, s, len);
	val->key.raw[len] = '\0';
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	val->key.s4k = s4k;
	memcpy(val->key.s4k->str, s, len);
	val->key.s4k->str[len] = '\0';
    } else {
	val->key.ptr = (char*)OJ_MALLOC(len + 1);
	val->key.cap = len + 1;
	memcpy(val->key.ptr, s, len);
	val->key.ptr[len] = '\0';
    }
    val->key.len = len;
}

void
_oj_val_set_str(ojVal val, const char *s, size_t len) {
    if (len < sizeof(val->str.raw)) {
	memcpy(val->str.raw, s, len);
	val->str.raw[len] = '\0';
    } else if (len < sizeof(union ojS4k)) {
	union ojS4k	*s4k = s4k_create();

	val->str.s4k = s4k;
	memcpy(val->str.s4k->str, s, len);
	val->str.s4k->str[len] = '\0';
    } else {
	val->str.ptr = (char*)OJ_MALLOC(len + 1);
	memcpy(val->str.ptr, s, len);
	val->str.ptr[len] = '\0';
    }
    val->str.len = len;
}

void
_oj_append_num(ojParser p, const char *s, size_t len) {
    size_t	nl = p->stack->num.len + len;

    if (p->stack->num.len < sizeof(p->stack->num.raw)) {
	if (nl < sizeof(p->stack->num.raw)) {
	    memcpy(p->stack->num.raw + p->stack->num.len, s, len);
	    p->stack->num.raw[nl] = '\0';
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)OJ_MALLOC(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(&p->err, "number");
		p->stack->num.len = 0;
		return;
	    }
	    memcpy(ptr, p->stack->num.raw, p->stack->num.len);
	    memcpy(ptr + p->stack->num.len, s, len);
	    ptr[nl] = '\0';
	    p->stack->num.cap = cap;
	    p->stack->num.ptr = ptr;
	}
    } else {
	if (nl < p->stack->num.cap) {
	    memcpy(p->stack->num.ptr + p->stack->num.len, s, len);
	} else {
	    p->stack->num.cap = nl * 3 / 2;
	    if (NULL == (p->stack->num.ptr = OJ_REALLOC(p->stack->num.ptr, p->stack->num.cap))) {
		OJ_ERR_MEM(&p->err, "string");
		p->stack->num.len = 0;
		return;
	    } else {
		memcpy(p->stack->num.ptr + p->stack->num.len, s, len);
	    }
	}
	p->stack->num.ptr[nl] = '\0';
    }
    p->stack->num.len = nl;
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
	    char	*ptr = (char*)OJ_MALLOC(cap);

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
	    char	*ptr = (char*)OJ_MALLOC(cap);

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
	    if (NULL == (str->ptr = OJ_REALLOC(str->ptr, str->cap))) {
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
    _oj_append_str(p, &p->stack->str, s, len);
}
