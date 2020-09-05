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

static ojVal		volatile free_head = NULL;
static ojVal		volatile free_tail = NULL;
static atomic_flag	val_busy = ATOMIC_FLAG_INIT;

static ojS4k		volatile s4k_head = NULL;
static ojS4k		volatile s4k_tail = NULL;
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

ojS4k
s4k_create() {
    union _ojS4k	*s = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == s4k_head) {
	s = (ojS4k)OJ_CALLOC(1, sizeof(union _ojS4k));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	if (oj_thread_safe) {
	    while (atomic_flag_test_and_set(&s4k_busy)) {
	    }
	}
	if (NULL == s4k_head) {
	    s = (ojS4k)OJ_CALLOC(1, sizeof(union _ojS4k));
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
    ojS4k	s4k;

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

static void
clear_key(ojVal v) {
    ojS4k	s4k_h = NULL;
    ojS4k	s4k_t = NULL;

    if (sizeof(union _ojS4k) < v->key.len) {
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

static void
clear_value(ojVal v) {
    ojS4k	s4k_h = NULL;
    ojS4k	s4k_t = NULL;

    switch (v->type) {
    case OJ_STRING:
	if (sizeof(union _ojS4k) < v->str.len) {
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

// code copied from clear_key and clear_value btu separate to avoid locking
// the s4k head and tail twice.
void
_oj_val_clear(ojVal v) {
    ojS4k	s4k_h = NULL;
    ojS4k	s4k_t = NULL;

    if (sizeof(union _ojS4k) < v->key.len) {
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
	if (sizeof(union _ojS4k) < v->str.len) {
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
    ojS4k	s4k_h = NULL;
    ojS4k	s4k_t = NULL;

    for (v = reuser->dig; NULL != v; v = next) {
	next = v->free;
	if (sizeof(union _ojS4k) < v->key.len) {
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
	    if (sizeof(union _ojS4k) < v->str.len) {
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
    ojS4k	s4k_h = NULL;
    ojS4k	s4k_t = NULL;

    if (NULL == val) {
	return;
    }
    val->free = NULL;
    for (; NULL != v; v = v->free) {
	if (sizeof(union _ojS4k) < v->key.len) {
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
	    if (sizeof(union _ojS4k) < v->str.len) {
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
	v->type = OJ_NONE;
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

//// set functions

void
oj_null_set(ojVal val) {
    clear_value(val);
    val->type = OJ_NULL;
}

void
oj_bool_set(ojVal val, bool b) {
    clear_value(val);
    val->type = (b ? OJ_TRUE : OJ_FALSE);
}

void
oj_int_set(ojVal val, int64_t fixnum) {
    clear_value(val);
    val->type = OJ_INT;
    val->num.fixnum = fixnum;
    val->num.len = 0;
}

void
oj_double_set(ojVal val, long double dub) {
    clear_value(val);
    val->type = OJ_DECIMAL;
    val->num.dub = dub;
    val->num.len = 0;
}

ojStatus
oj_str_set(ojErr err, ojVal val, const char *s, size_t len) {
    clear_value(val);
    val->type = OJ_STRING;
    if (len < sizeof(val->str.raw)) {
	memcpy(val->str.raw, s, len);
	val->str.raw[len] = '\0';
    } else if (len < sizeof(union _ojS4k)) {
	ojS4k	s4k = s4k_create();

	val->str.s4k = s4k;
	memcpy(val->str.s4k->str, s, len);
	val->str.s4k->str[len] = '\0';
    } else {
	if (NULL == (val->str.ptr = (char*)OJ_MALLOC(len + 1))) {
	    return OJ_ERR_MEM(err, "string");
	}
	val->str.cap = len + 1;
	memcpy(val->str.ptr, s, len);
	val->str.ptr[len] = '\0';
    }
    return OJ_OK;
}

// Setting the key when the val is in an object in hash mode will corrupt the hash.
ojStatus
oj_key_set(ojErr err, ojVal val, const char *s, size_t len) {
    clear_key(val);
    val->key.len = len;
    if (len < sizeof(val->key.raw)) {
	memcpy(val->key.raw, s, len);
	val->key.raw[len] = '\0';
    } else if (len < sizeof(union _ojS4k)) {
	ojS4k	s4k = s4k_create();

	val->key.s4k = s4k;
	memcpy(val->key.s4k->str, s, len);
	val->key.s4k->str[len] = '\0';
    } else {
	if (NULL == (val->key.ptr = (char*)OJ_MALLOC(len + 1))) {
	    return OJ_ERR_MEM(err, "string");
	}
	val->key.cap = len + 1;
	memcpy(val->key.ptr, s, len);
	val->key.ptr[len] = '\0';
    }
    return OJ_OK;
}

ojStatus
oj_bignum_set(ojErr err, ojVal val, const char *s, size_t len) {
    // TBD validate string first
    clear_value(val);
    val->type = OJ_BIG;
    if (len < sizeof(val->num.raw)) {
	memcpy(val->num.raw, s, len);
	val->num.raw[len] = '\0';
    } else if (len < sizeof(union _ojS4k)) {
	ojS4k	s4k = s4k_create();

	val->num.s4k = s4k;
	memcpy(val->num.s4k->str, s, len);
	val->num.s4k->str[len] = '\0';
    } else {
	if (NULL == (val->num.ptr = (char*)OJ_MALLOC(len + 1))) {
	    return OJ_ERR_MEM(err, "string");
	}
	val->num.cap = len + 1;
	memcpy(val->num.ptr, s, len);
	val->num.ptr[len] = '\0';
    }
    return OJ_OK;
}

ojStatus
oj_append(ojErr err, ojVal val, ojVal member) {
    if (NULL == val || NULL == member) {
	return oj_err_set(err, OJ_ERR_ARG, "can not append null");
    }
    switch (val->type) {
    case OJ_ARRAY:
	val->next = NULL;
	if (NULL == val->list.head) {
	    val->list.head = val;
	} else {
	    val->list.tail->next = val;
	}
	val->list.tail = val;
	break;
    case OJ_OBJECT:
	if (0 == member->key.len) {
	    return oj_err_set(err, OJ_ERR_KEY, "appending to an object requires the member to have a key");
	}
	if (OJ_OBJ_HASH == val->mod) {
	    member->kh = calc_hash(oj_key(member), member->key.len);
	    member->next = val->hash[member->kh & 0x0000000F];
	    val->hash[member->kh & 0x0000000F] = member;
	} else {
	    val->next = NULL;
	    if (NULL == val->list.head) {
		val->list.head = val;
	    } else {
		val->list.tail->next = val;
	    }
	    val->list.tail = val;
	}
	break;
    default:
	return oj_err_set(err, OJ_ERR_TYPE, "can not append to a %s", oj_type_str(val->type));
    }
    return OJ_OK;
}

ojStatus
oj_object_set(ojErr err, ojVal val, const char *key, ojVal member) {
    if (OJ_OBJECT != val->type) {
	return oj_err_set(err, OJ_ERR_TYPE, "can not perform an object set on a %s", oj_type_str(val->type));
    }
    if (OJ_OK != oj_key_set(err, val, key, strlen(key))) {
	return err->code;
    }
    if (OJ_OBJ_HASH == val->mod) {
	member->kh = calc_hash(oj_key(member), member->key.len);
	member->next = val->hash[member->kh & 0x0000000F];
	val->hash[member->kh & 0x0000000F] = member;
    } else {
	val->next = NULL;
	if (NULL == val->list.head) {
	    val->list.head = val;
	} else {
	    val->list.tail->next = val;
	}
	val->list.tail = val;
    }
    return OJ_OK;
}

//// create functions

ojVal
oj_null_create(ojErr err) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_NULL;
    }
    return val;
}

ojVal
oj_bool_create(ojErr err, bool b) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = (b ? OJ_TRUE : OJ_FALSE);
    }
    return val;
}

ojVal
oj_str_create(ojErr err, const char *s, size_t len) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_NONE;
	if (OJ_OK != oj_str_set(err, val, s, len)) {
	    val = NULL;
	}
    }
    return val;
}

ojVal
oj_int_create(ojErr err, int64_t fixnum) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_INT;
	val->num.fixnum = fixnum;
	val->num.len = 0;
    }
    return val;
}

ojVal
oj_double_create(ojErr err, long double dub) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_DECIMAL;
	val->num.dub = dub;
	val->num.len = 0;
    }
    return val;
}

ojVal
oj_bignum_create(ojErr err, const char *s, size_t len) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_NONE;
	if (OJ_OK != oj_bignum_set(err, val, s, len)) {
	    val = NULL;
	}
    }
    return val;
}

ojVal
oj_array_create(ojErr err) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_ARRAY;
	val->list.head = NULL;
	val->list.tail = NULL;
    }
    return val;
}

ojVal
oj_object_create(ojErr err) {
    ojVal	val = oj_val_create();

    if (NULL == val) {
	OJ_ERR_MEM(err, "ojVal");
    } else {
	val->type = OJ_OBJECT;
	val->mod = OJ_OBJ_HASH;
	for (ojVal *bucket = val->hash + sizeof(val->hash) / sizeof(*val->hash) - 1; val->hash <= bucket; bucket--) {
	    *bucket = NULL;
	}
    }
    return val;
}

const char*
oj_key(ojVal val) {
    const char	*k = NULL;

    if (NULL != val) {
	if (val->key.len < sizeof(val->key.raw)) {
	    k = val->key.raw;
	} else if (val->key.len < sizeof(union _ojS4k)) {
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
	} else if (val->str.len < sizeof(union _ojS4k)) {
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

// has to be a OJ_OBJ_HASH
static ojVal
object_get(ojVal val, const char *key, int len) {
    uint32_t	kh = calc_hash(key, len);
    ojVal	v = val->hash[kh & 0x0000000F];

    for (; NULL != v; v = v->next) {
	if (v->kh == kh && len == v->key.len && 0 == strncmp(key, oj_key(v), len)) {
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
	    uint32_t	u;
	    ojVal	next;

	    v = val->list.head;
	    memset(val->hash, 0, sizeof(val->hash));
	    for (; NULL != v; v = next) {
		next = v->next;
		v->kh = calc_hash(oj_key(v), v->key.len);
		u = v->kh & 0x0000000F;
		v->next = val->hash[u];
		val->hash[u] = v;
	    }
	    val->mod = OJ_OBJ_HASH;
	}
	v = object_get(val, key, len);
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
	    v = object_get(val, key, len);
	}
    }
    return v;
}

// TBD candidates for common function
void
_oj_val_set_key(ojVal val, const char *s, size_t len) {
    if (len < sizeof(val->key.raw)) {
	memcpy(val->key.raw, s, len);
	val->key.raw[len] = '\0';
    } else if (len < sizeof(union _ojS4k)) {
	ojS4k	s4k = s4k_create();

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
    } else if (len < sizeof(union _ojS4k)) {
	ojS4k	s4k = s4k_create();

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
_oj_append_num(ojErr err, ojNum num, const char *s, size_t len) {
    size_t	nl = num->len + len;

    if (num->len < sizeof(num->raw)) {
	if (nl < sizeof(num->raw)) {
	    memcpy(num->raw + num->len, s, len);
	    num->raw[nl] = '\0';
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)OJ_MALLOC(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(err, "number");
		num->len = 0;
		return;
	    }
	    memcpy(ptr, num->raw, num->len);
	    memcpy(ptr + num->len, s, len);
	    ptr[nl] = '\0';
	    num->cap = cap;
	    num->ptr = ptr;
	}
    } else {
	if (nl < num->cap) {
	    memcpy(num->ptr + num->len, s, len);
	} else {
	    num->cap = nl * 3 / 2;
	    if (NULL == (num->ptr = OJ_REALLOC(num->ptr, num->cap))) {
		OJ_ERR_MEM(err, "string");
		num->len = 0;
		return;
	    } else {
		memcpy(num->ptr + num->len, s, len);
	    }
	}
	num->ptr[nl] = '\0';
    }
    num->len = nl;
}

void
_oj_append_str(ojErr err, ojStr str, const byte *s, size_t len) {
    size_t	nl = str->len + len;

    if (str->len < sizeof(str->raw)) {
	if (nl < sizeof(str->raw)) {
	    memcpy(str->raw + str->len, s, len);
	    str->raw[nl] = '\0';
	} else if (nl < sizeof(union _ojS4k)) {
	    ojS4k	s4k = s4k_create();

	    memcpy(s4k->str, str->raw, str->len);
	    memcpy(s4k->str + str->len, s, len);
	    s4k->str[nl] = '\0';
	    str->s4k = s4k;
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)OJ_MALLOC(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(err, "string");
		str->len = 0;
		return;
	    }
	    memcpy(ptr, str->raw, str->len);
	    memcpy(ptr + str->len, s, len);
	    ptr[nl] = '\0';
	    str->cap = cap;
	    str->ptr = ptr;
	}
    } else if (str->len < sizeof(union _ojS4k)) {
	if (nl < sizeof(union _ojS4k)) {
	    memcpy(str->s4k->str + str->len, s, len);
	    str->s4k->str[nl] = '\0';
	} else {
	    size_t	cap = nl * 3 / 2;
	    char	*ptr = (char*)OJ_MALLOC(cap);

	    if (NULL == ptr) {
		OJ_ERR_MEM(err, "string");
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
		OJ_ERR_MEM(err, "string");
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
