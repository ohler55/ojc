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
write_jsons(struct _data *dp) {
    struct _ojBuf	buf;
    struct _ojErr	err;
    ojVal		val;
    bool		ok;

    if (ut_verbose) {
	ut_print("%s\n", ut_name());
    }
    for (; NULL != dp->src; dp++) {
	oj_err_init(&err);
	val = oj_parse_str(&err, dp->src, NULL);
	if (ut_handle_oj_error(&err)) {
	    ut_print("error at %d:%d for '%s'\n",  err.line, err.col, dp->src);
	    return;
	}
	oj_buf_init(&buf, 0);
	oj_buf(&buf, val, 0, 0);
	if (!ut_same(dp->src, buf.head)) {
	    break;
	}
	oj_buf_init(&buf, 0);
	oj_buf(&buf, val, 2, 0);
	ok = ut_same(dp->out2, buf.head);
	if (ut_verbose) {
	    const char	*s = dp->src;
	    char	tmp[64];

	    if (sizeof(tmp) - 4 < strlen(dp->src)) {
		char	*t;

		memcpy(tmp, dp->src, sizeof(tmp) - 4);
		t = tmp + sizeof(tmp) - 4;
		*t++ = '.';
		*t++ = '.';
		*t++ = '.';
		*t++ = '\0';
		s = tmp;
	    }
	    if (ok) {
		ut_print("--- '%s' - \033[92mpass\033[0m\n", s);
	    } else {
		ut_print("--- '%s' - \033[91mfail\033[0m\n", s);
	    }
	}
	oj_buf_cleanup(&buf);
	oj_destroy(val);
    }
}

static void
write_null_test() {
    struct _data	cases[] = {
	{.src = "null", .out2 = "null" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_bool_test() {
    struct _data	cases[] = {
	{.src = "true", .out2 = "true" },
	{.src = "false", .out2 = "false" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_string_test() {
    struct _data	cases[] = {
	{.src = "\"abc\"", .out2 = "\"abc\"" },
	{.src = "\"a\\nbc\"", .out2 = "\"a\\nbc\"" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_number_test() {
    struct _data	cases[] = {
	{.src = "123", .out2 = "123" },
	{.src = "-123", .out2 = "-123" },
	{.src = "-1.23", .out2 = "-1.23" },
	{.src = "-1.2e+300", .out2 = "-1.2e+300" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_array_test() {
    struct _data	cases[] = {
	{.src = "[]", .out2 = "[\n]" },
	{.src = "[true]", .out2 = "[\n  true\n]" },
	{.src = "[true,false]", .out2 = "[\n  true,\n  false\n]" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_object_test() {
    struct _data	cases[] = {
	{.src = "{}", .out2 = "{\n}" },
	{.src = "{\"x\":123}", .out2 = "{\n  \"x\":123\n}" },
	{.src = "{\"x\":123,\"y\":true}", .out2 = "{\n  \"x\":123,\n  \"y\":true\n}" },
	{.src = NULL }};

    write_jsons(cases);
}

static void
write_mixed_test() {
    struct _data	cases[] = {
	{.src = "{\"x\":[123,{\"y\":[]}]}", .out2 = "{\n  \"x\":[\n    123,\n    {\n      \"y\":[\n      ]\n    }\n  ]\n}" },
	{.src = NULL }};

    write_jsons(cases);
}

void
append_write_tests(Test tests) {
    ut_append(tests, "write.null", write_null_test);
    ut_append(tests, "write.bool", write_bool_test);
    ut_append(tests, "write.string", write_string_test);
    ut_append(tests, "write.number", write_number_test);
    ut_append(tests, "write.array", write_array_test);
    ut_append(tests, "write.object", write_object_test);
    ut_append(tests, "write.mixed", write_mixed_test);
}
