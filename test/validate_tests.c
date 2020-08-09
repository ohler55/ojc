// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "ut.h"

struct _data {
    const char	*json;
    ojStatus	status;
};

static void
validate_jsons(struct _data *dp) {
    struct _ojErr	err = OJ_ERR_INIT;
    ojStatus    	status;

    for (; NULL != dp->json; dp++) {
	status = oj_validate_str(&err, dp->json);
	if (OJ_OK == dp->status) {
	    if (ut_handle_oj_error(&err)) {
		ut_print("error at %d:%d\n",  err.line, err.col);
		return;
	    }
	} else if (status != dp->status) {
	    ut_print("%s: expected error [%d], not [%d] %s\n", dp->json, dp->status, err.code, err.msg);
	    ut_fail();
	}
    }
}

static void
validate_null_test() {
    struct _data	cases[] = {
	{.json = "null", .status = OJ_OK },
	{.json = "  null", .status = OJ_OK },
	{.json = "\n null", .status = OJ_OK },
	{.json = "nuLL", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_true_test() {
    struct _data	cases[] = {
	{.json = "true", .status = OJ_OK },
	{.json = "  true\n ", .status = OJ_OK },
	{.json = "trUe", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_false_test() {
    struct _data	cases[] = {
	{.json = "false", .status = OJ_OK },
	{.json = "  false", .status = OJ_OK },
	{.json = "faLse", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_string_test() {
    struct _data	cases[] = {
	{.json = "\"abc\"", .status = OJ_OK },
	{.json = "  \"abc\" ", .status = OJ_OK },
	{.json = "\"a\\tb\\u00e9c\"", .status = OJ_OK },
	{.json = "\"a\nb\"", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_number_test() {
    struct _data	cases[] = {
	{.json = "0", .status = OJ_OK },
	{.json = "0 ", .status = OJ_OK },
	{.json = "0\n ", .status = OJ_OK },
	{.json = "123", .status = OJ_OK },
	{.json = "-123", .status = OJ_OK },
	{.json = "1.23", .status = OJ_OK },
	{.json = "-1.23", .status = OJ_OK },
	{.json = "-1.23e1", .status = OJ_OK },
	{.json = "1.2.3", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_array_test() {
    struct _data	cases[] = {
	{.json = "[]", .status = OJ_OK },
	{.json = "[true]", .status = OJ_OK },
	{.json = "[true,false]", .status = OJ_OK },
	{.json = "[123 , 456]", .status = OJ_OK },
	{.json = "[[], [ ]]", .status = OJ_OK },
	{.json = "[}", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_object_test() {
    struct _data	cases[] = {
	{.json = "{}", .status = OJ_OK },
	{.json = "{\"x\":true}", .status = OJ_OK },
	{.json = "{\"x\":1,\"y\":0}", .status = OJ_OK },
	{.json = "{]", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    validate_jsons(cases);
}

static void
validate_mixed_test() {
    struct _data	cases[] = {
	{.json = "{\"x\":true}{\"y\":false}", .status = OJ_OK },
	{.json = NULL }};

    validate_jsons(cases);
}

void
append_validate_tests(Test tests) {
    ut_append(tests, "validate.null", validate_null_test);
    ut_append(tests, "validate.true", validate_true_test);
    ut_append(tests, "validate.false", validate_false_test);
    ut_append(tests, "validate.string", validate_string_test);
    ut_append(tests, "validate.number", validate_number_test);
    ut_append(tests, "validate.array", validate_array_test);
    ut_append(tests, "validate.object", validate_object_test);
    ut_append(tests, "validate.mixed", validate_mixed_test);
}
