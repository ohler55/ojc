// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "ut.h"

struct _data {
    const char	*src;
    const char	*out2;
};

static void
build_test() {
    struct _ojBuilder	b = OJ_BUILDER_INIT;

    oj_build_object(&b, NULL);
    oj_build_double(&b, "num", 12.34e567L);
    oj_build_int(&b, "fix", 1234567);
    oj_build_bool(&b, "boo", true);
    oj_build_null(&b, "nil");
    oj_build_object(&b, "obj");
    oj_build_pop(&b);
    oj_build_array(&b, "list");
    oj_build_bool(&b, NULL, true);
    oj_build_bool(&b, NULL, false);
    oj_build_null(&b, NULL);
    oj_build_int(&b, NULL, 1234);
    oj_build_double(&b, NULL, 1.25);
    oj_build_object(&b, NULL);
    oj_build_popall(&b);

    if (OJ_OK != b.err.code) {
	if (ut_handle_oj_error(&b.err)) {
	    ut_print("error at %d:%d\n",  b.err.line, b.err.col);
	}
	return;
    }
    char		buf[256];
    struct _ojErr	err = OJ_ERR_INIT;

    oj_fill(&err, b.top, 2, buf, sizeof(buf));
    ut_same("{\n"
	    "  \"num\":1.234e+568,\n"
	    "  \"fix\":1234567,\n"
	    "  \"boo\":true,\n"
	    "  \"nil\":null,\n"
	    "  \"obj\":{\n"
	    "  },\n"
	    "  \"list\":[\n"
	    "    true,\n"
	    "    false,\n"
	    "    null,\n"
	    "    1234,\n"
	    "    1.25,\n"
	    "    {\n"
	    "    }\n"
	    "  ]\n"
	    "}", buf);
    oj_destroy(b.top);
}

void
append_build_tests(Test tests) {
    ut_append(tests, "build", build_test);
}
