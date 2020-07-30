// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <stdatomic.h>

#include "oj.h"
#include "buf.h"

static struct _ojList	free_vals = { .head = NULL, .tail = NULL };
static atomic_flag	val_busy = ATOMIC_FLAG_INIT;

ojVal
oj_val_create() {
    ojVal	val = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == free_vals.head || free_vals.head == free_vals.tail) {
	val = (ojVal)calloc(1, sizeof(struct _ojVal));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	while (atomic_flag_test_and_set(&val_busy)) {
	}
	if (NULL == free_vals.head || free_vals.head == free_vals.tail) {
	    val = (ojVal)calloc(1, sizeof(struct _ojVal));
	} else {
	    val = free_vals.head;
	    free_vals.head = free_vals.head->next;
	}
	atomic_flag_clear(&val_busy);
    }
    return val;
}

// TBD redo this to handle batches
ojStatus
oj_destroy(ojVal val) {
    if (NULL != val) {
	if (OJ_FREE == val->type) {
	    printf("*** already freed\n");
	    return OJ_ERR_MEMORY;
	}
    }
    switch (val->type) {
    case OJ_STRING:
	break;
    case OJ_INT:
	// TBD if raw and ext then free the exts
	break;
    case OJ_DECIMAL:
	// TBD if raw and ext then free the exts
	break;
    case OJ_OBJECT:
	if (OJ_OBJ_RAW == val->mod) {
	    ojVal	next = NULL;
	    ojVal	v;
	    ojStatus	status;

	    for (v = val->list; NULL != v; v = next) {
		next = v->next;
		if (OJ_OK != (status = oj_destroy(v))) {
		    return status;
		}
	    }
	} else {
	    // TBD each hash bucket
	}
	break;
    case OJ_ARRAY: {
	ojVal		next = NULL;
	ojVal		v;
	ojStatus	status;

	for (v = val->list; NULL != v; v = next) {
	    next = v->next;
	    if (OJ_OK != (status = oj_destroy(v))) {
		return status;
	    }
	}
	break;
    }
    default:
	break;
    }
    if (NULL == free_vals.head) {
	free_vals.head = val;
    } else {
	free_vals.tail->next = val;
    }
    val->next = NULL;
    val->type = OJ_FREE;
    free_vals.tail = val;

    return OJ_OK;
}

typedef struct _Stack {
    // TBD change to be expandable
    ojVal	vals[256];
    int		depth;
} *Stack;

static void
push(ojVal val, void *ctx) {
    //Stack	stack = (Stack)ctx;

    // val create, copy/set
    // TBD if end of stack is obj or array then add to list tail (for poc, the head is ok)
    //
}

static void
pop(void *ctx) {
    ((Stack)ctx)->depth--;
}

void
oj_val_parser(ojParser p) {
    // TBD is this needed?
    p->push = push;
    p->pop = pop;
    p->cb = NULL; // TBD set callback to set final result
    p->cb_ctx = NULL; // TBD set to what ever is needed
    //p->pp_ctx = &stack;
    oj_err_init(&p->err);
}

ojVal
oj_val_parse_str(ojErr err, const char *json) {
    struct _ojParser	p;
    struct _Stack	stack = { .vals = { NULL }, .depth = 0 };

    memset(&p, 0, sizeof(p));
    p.push = push;
    p.pop = pop;
    p.pp_ctx = &stack;

    oj_parse_str(&p, json);

    return stack.vals[0];
}

ojVal
oj_val_parse_strp(ojErr err, const char **json) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_file(ojErr err, FILE *file) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_fd(ojErr err, int socket) {

    // TBD

    return NULL;
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
		oj_buf_append_string(buf, val->str.val + 1, (size_t)*val->str.val);
	    } else {
		// TBD
	    }
	    break;
	case OJ_DECIMAL:
	    if (OJ_DEC_RAW == val->mod) {
		oj_buf_append_string(buf, val->str.val + 1, (size_t)*val->str.val);
	    } else {
		// TBD
	    }
	    break;
	case OJ_STRING:
	    oj_buf_append(buf, '"');
	    // TBD handle extended
	    oj_buf_append_string(buf, val->str.val + 1, (size_t)*val->str.val);
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
