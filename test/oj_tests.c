// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj/oj.h"
#include "ut.h"

struct _data {
    const char	*json;
    ojStatus	status;
};

static void
test_jsons(struct _data *dp) {
    struct _ojValidator	v;
    ojStatus    	status;

    for (; NULL != dp->json; dp++) {
	printf("*** test json: '%s'\n", dp->json);
	status = oj_validate_str(&v, dp->json);
	if (OJ_OK == dp->status) {
	    if (ut_handle_oj_error(&v.err)) {
		ut_print("error at %d:%d\n",  v.err.line, v.err.col);
		return;
	    }
	} else if (status != dp->status) {
	    ut_print("%s: expected error [%d], not [%d] %s\n", dp->json, dp->status, v.err.code, v.err.msg);
	    ut_fail();
	}
    }
}

static void
null_test() {
    struct _data	cases[] = {
	{.json = "null", .status = OJ_OK },
	{.json = "  null", .status = OJ_OK },
	{.json = "\n null", .status = OJ_OK },
	{.json = "nuLL", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static void
true_test() {
    struct _data	cases[] = {
	{.json = "true", .status = OJ_OK },
	{.json = "  true\n ", .status = OJ_OK },
	{.json = "trUe", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static void
false_test() {
    struct _data	cases[] = {
	{.json = "false", .status = OJ_OK },
	{.json = "  false", .status = OJ_OK },
	{.json = "faLse", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static void
string_test() {
    struct _data	cases[] = {
	{.json = "\"abc\"", .status = OJ_OK },
	{.json = "  \"abc\" ", .status = OJ_OK },
	{.json = "\"a\nb\"", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static void
number_test() {
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

    test_jsons(cases);
}

static void
array_test() {
    struct _data	cases[] = {
	{.json = "[]", .status = OJ_OK },
	{.json = "[true]", .status = OJ_OK },
	{.json = "[true,false]", .status = OJ_OK },
	{.json = "[123 , 456]", .status = OJ_OK },
	{.json = "[[], [ ]]", .status = OJ_OK },
	{.json = "[}", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static void
object_test() {
    struct _data	cases[] = {
	{.json = "{}", .status = OJ_OK },
	{.json = "{\"x\":true}", .status = OJ_OK },
	{.json = "{\"x\":1,\"y\":0}", .status = OJ_OK },
	{.json = "{]", .status = OJ_ERR_PARSE },
	{.json = NULL }};

    test_jsons(cases);
}

static struct _Test	tests[] = {
    { "null",		null_test },
    { "true",		true_test },
    { "false",		false_test },
    { "string",		string_test },
    { "number",		number_test },
    { "array",		array_test },
    { "object",		object_test },

    { 0, 0 } };

int
main(int argc, char **argv) {
    ut_init(argc, argv, "oj", tests);

    ut_done();

    return 0;
}
