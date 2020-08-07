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
    struct _ojParser	p;
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    memset(&p, 0, sizeof(p));
    p.ctx = &buf;
    p.push = push;
    p.pop = pop;
    for (; NULL != dp->json; dp++) {
	oj_parser_reset(&p);
	oj_buf_reset(&buf);
	status = oj_parse_str(&p, dp->json);
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

static void
parse_jsons(struct _data *dp) {
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;
    struct _ojBuf	buf;

    for (; NULL != dp->json; dp++) {
	val = oj_val_parse_str(&err, dp->json, NULL, NULL);
	if (OJ_OK == dp->status) {
	    bool	ok;

	    if (ut_handle_oj_error(&err)) {
		ut_print("error at %d:%d\n",  err.line, err.col);
		return;
	    }
	    oj_buf_init(&buf, 0);
	    oj_buf(&buf, val, 0, 0);
	    if (NULL == dp->expect) {
		ok = ut_same(dp->json, buf.head);
	    } else {
		ok = ut_same(dp->expect, buf.head);
	    }
	    if (ok && ut_verbose) {
		const char	*s = dp->json;
		char		tmp[64];

		if (sizeof(tmp) - 4 < strlen(dp->json)) {
		    char	*t;

		    memcpy(tmp, dp->json, sizeof(tmp) - 4);
		    t = tmp + sizeof(tmp) - 4;
		    *t++ = '.';
		    *t++ = '.';
		    *t++ = '.';
		    *t++ = '\0';
		    s = tmp;
		}
		ut_print("--- '%s' - pass\n", s);
	    }
	    oj_buf_cleanup(&buf);
	    oj_destroy(val);
	} else if (err.code != dp->status) {
	    ut_print("%s: expected error [%d], not [%d] %s\n", dp->json, dp->status, err.code, err.msg);
	    ut_fail();
	}
    }
}

static void
parse_test() {
    char	big[128];
    char	big_cut[128];

    *big = '"';
    for (int i = 0; i < sizeof(big) - 2; i++) {
	big[i+1] = 'a' + i % 26;
    }
    big[sizeof(big) - 2] = '"';
    big[sizeof(big) - 1] = '\0';
    memcpy(big_cut, big, sizeof(big));
    big_cut[1] = 'x';
    big_cut[57] = '\\';
    big_cut[58] = 't';

    char	bigger[5000];
    char	bigger_cut[5000];

    *bigger = '"';
    for (int i = 0; i < sizeof(bigger) - 2; i++) {
	bigger[i+1] = 'A' + i % 26;
    }
    bigger[sizeof(bigger) - 2] = '"';
    bigger[sizeof(bigger) - 1] = '\0';
    memcpy(bigger_cut, bigger, sizeof(bigger));
    bigger_cut[1] = 'x';
    bigger_cut[130] = '\\';
    bigger_cut[131] = 't';

    struct _data	cases[] = {
	{.json = "\"abc\"", .status = OJ_OK },
	{.json = "\"ab\\tcd\"", .status = OJ_OK },
	{.json = big, .status = OJ_OK },
	{.json = big_cut, .status = OJ_OK },
	{.json = bigger, .status = OJ_OK },
	{.json = bigger_cut, .status = OJ_OK },

	{.json = "{\"x\":true,\"y\":false}", .status = OJ_OK },
	{.json = NULL }};

    parse_jsons(cases);
}

static struct _Test	tests[] = {
    { "validate.null",		validate_null_test },
    { "validate.true",		validate_true_test },
    { "validate.false",		validate_false_test },
    { "validate.string",	validate_string_test },
    { "validate.number",	validate_number_test },
    { "validate.array",		validate_array_test },
    { "validate.object",	validate_object_test },
    { "validate.mixed",		validate_mixed_test },

    { "push-pop",		push_pop_test },

    { "parse",			parse_test },

    { 0, 0 } };

int
main(int argc, char **argv) {
    ut_init(argc, argv, "oj", tests);

    ut_done();
    oj_cleanup();

    return 0;
}
