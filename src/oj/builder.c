// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "oj.h"
#include "intern.h"

static ojStatus
push(ojBuilder b, const char *key, ojVal v) {
    if (NULL != v && OJ_OK == b->err.code) {
	if (NULL == b->top) {
	    b->top = v;
	    v->next = NULL;
	    b->stack = v;
	} else if (NULL == b->stack) {
	    oj_err_set(&b->err, OJ_ERR_TOO_MANY, "can not push after all elements have been closed");
	} else {
	    ojVal	parent = b->stack;

	    switch (parent->type) {
	    case OJ_ARRAY:
		if (NULL != key) {
		    oj_err_set(&b->err, OJ_ERR_KEY, "a key is not needed to push to an array");
		    break;
		}
		v->key.len = 0;
		if (NULL == parent->list.head) {
		    parent->list.head = v;
		} else {
		    parent->list.tail->next = v;
		}
		parent->list.tail = v;
		if (OJ_OBJECT == v->type || OJ_ARRAY == v->type) {
		    v->next = b->stack;
		    b->stack = v;
		}
		break;
	    case OJ_OBJECT:
		if (NULL == key) {
		    oj_err_set(&b->err, OJ_ERR_KEY, "a key is required to push to an object");
		    break;
		}
		if (OJ_OK != oj_key_set(&b->err, v, key, strlen(key))) {
		    break;
		}
		if (OJ_OBJ_RAW != parent->mod) {
		    oj_err_set(&b->err, OJ_ERR_TYPE, "object value mod has been changed");
		    break;
		}
		if (NULL == parent->list.head) {
		    parent->list.head = v;
		} else {
		    parent->list.tail->next = v;
		}
		parent->list.tail = v;
		if (OJ_OBJECT == v->type || OJ_ARRAY == v->type) {
		    v->next = b->stack;
		    b->stack = v;
		}
		break;
	    default:
		oj_err_set(&b->err, OJ_ERR_TYPE, "can not push onto a %s", oj_type_str(b->top->type));
		break;
	    }
	}
    }
    return b->err.code;
}

ojStatus
oj_build_object(ojBuilder b, const char *key) {
    ojVal	v = oj_object_create(&b->err);

    if (NULL != v) {
	v->mod = OJ_OBJ_RAW;
    }
    return push(b, key, v);
}

ojStatus
oj_build_array(ojBuilder b, const char *key) {
    return push(b, key, oj_array_create(&b->err));
}

ojStatus
oj_build_null(ojBuilder b, const char *key) {
    return push(b, key, oj_null_create(&b->err));
}

ojStatus
oj_build_bool(ojBuilder b, const char *key, bool boo) {
    return push(b, key, oj_bool_create(&b->err, boo));
}

ojStatus
oj_build_int(ojBuilder b, const char *key, int64_t i) {
    return push(b, key, oj_int_create(&b->err, i));
}

ojStatus
oj_build_double(ojBuilder b, const char *key, long double d) {
    return push(b, key, oj_double_create(&b->err, d));
}

ojStatus
oj_build_string(ojBuilder b, const char *key, const char *s, size_t len) {
    return push(b, key, oj_str_create(&b->err, s, len));
}

ojStatus
oj_build_bignum(ojBuilder b, const char *key, const char *big, size_t len) {
    return push(b, key, oj_str_create(&b->err, big, len));
}

ojStatus
oj_build_pop(ojBuilder b) {
    if (OJ_OK == b->err.code) {
	if (NULL == b->stack) {
	    oj_err_set(&b->err, OJ_ERR_TOO_MANY, "nothing left to pop");
	    return b->err.code;
	}
	ojVal	v = b->stack;

	b->stack = v->next;
	v->next = NULL;
    }
    return b->err.code;
}

void
oj_build_popall(ojBuilder b) {
    if (OJ_OK == b->err.code) {
	ojVal	v;
	while (NULL != b->stack) {
	    v = b->stack;
	    b->stack = v->next;
	    v->next = NULL;
	}
    }
}
