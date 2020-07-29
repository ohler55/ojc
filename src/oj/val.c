// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj.h"
#include "buf.h"

void
oj_val_parser(ojParser p) {
    p->push = NULL; // TBD set to val builder function
    p->pop = NULL; // TBD set to val builder function
    p->cb = NULL; // TBD set callback to set final result
    p->cb_ctx = NULL; // TBD set to what ever is needed
    p->pp_ctx = NULL; // TBD set to what ever is needed
    oj_err_init(&p->err);
}

ojVal
oj_val_parse_str(ojErr err, const char *json) {

    // TBD

    return NULL;
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
