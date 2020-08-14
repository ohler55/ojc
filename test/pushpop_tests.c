// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "ut.h"

struct _data {
    const char	*json;
    const char	*expect;
    ojStatus	status;
};

static void
push(ojVal val, void *ctx) {
    oj_buf((ojBuf)ctx, val, 0, 0);
    oj_buf_append((ojBuf)ctx, ' ');
}

static void
pop(void *ctx) {
    oj_buf_append_string((ojBuf)ctx, "pop ", 4);
}

static void
test_push_pop(struct _data *dp) {
    struct _ojErr	err = OJ_ERR_INIT;
    ojStatus    	status;
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    for (; NULL != dp->json; dp++) {
	status = oj_pp_parse_str(&err, dp->json, push, pop);
	if (OJ_OK == dp->status) {
	    if (ut_handle_oj_error(&err)) {
		ut_print("error at %d:%d\n",  err.line, err.col);
		return;
	    }
	} else if (status != dp->status) {
	    ut_print("%s: expected error [%d], not [%d] %s\n", dp->json, dp->status, err.code, err.msg);
	    ut_fail();
	}
	ut_same(dp->expect, buf.head);
    }
}

static void
push_pop_test() {
    struct _data	cases[] = {
	{.json = "{}", .status = OJ_OK, .expect = "{} pop " },
	{.json = "{\"x\":true}", .status = OJ_OK, .expect = "{} true pop " },
	{.json = "{\"x\":1,\"y\":0}", .status = OJ_OK, .expect = "{} 1 0 pop " },
	{.json = NULL }};

    test_push_pop(cases);
}

void
append_pushpop_tests(Test tests) {
    ut_append(tests, "push-pop", push_pop_test);
}
