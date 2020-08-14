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
parse_jsons(struct _data *dp) {
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;
    struct _ojBuf	buf;

    for (; NULL != dp->json; dp++) {
	val = oj_parse_str(&err, dp->json, NULL, NULL);
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
parse_mixed_test() {
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
/*
	{.json = "\"abc\"", .status = OJ_OK },
	{.json = "\"ab\\tcd\"", .status = OJ_OK },
	{.json = "\"\\u3074ー\\u305fー\"", .status = OJ_OK, .expect = "\"ぴーたー\"" },

	{.json = big, .status = OJ_OK },
	{.json = big_cut, .status = OJ_OK },
	{.json = bigger, .status = OJ_OK },
	{.json = bigger_cut, .status = OJ_OK },
*/
	{.json = "{\"x\":true,\"y\":false}", .status = OJ_OK },
	{.json = NULL }};

    parse_jsons(cases);
}

static void
parse_int_test() {
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;

    val = oj_parse_str(&err, "12345", NULL, NULL);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    int64_t	i = oj_val_get_int(val);

    ut_same_int(12345, i, "parse int");
    oj_destroy(val);
}

static void
parse_decimal_test() {
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;

    val = oj_parse_str(&err, "12.345", NULL, NULL);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    long double	d = oj_val_get_double(val, true);

    ut_same_double(12.345, d, 0.0001, "parse decimal");

    oj_destroy(val);
}

static void
parse_bignum_test() {
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;

    val = oj_parse_str(&err, "-9223372036854775807", NULL, NULL);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    const char	*num;
    int64_t	i = oj_val_get_int(val);

    ut_same_int(-9223372036854775807, i, "parse bignum");
    oj_destroy(val);

    val = oj_parse_str(&err, "9223372036854775808", NULL, NULL);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    i = oj_val_get_int(val);
    ut_same_int(0, i, "parse bignum");
    num = oj_val_get_bignum(val);
    ut_same("9223372036854775808", num);
    oj_destroy(val);

    val = oj_parse_str(&err, "-1.2e12345", NULL, NULL);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    long double	d = oj_val_get_double(val, true);

    ut_same_double(0.0, d, 0.0001, "parse bignum");
    num = oj_val_get_bignum(val);
    ut_same("-1.2e12345", num);

    oj_destroy(val);
}

void
append_parse_tests(Test tests) {
    ut_append(tests, "parse.mixed", parse_mixed_test);
    ut_append(tests, "parse.int", parse_int_test);
    ut_append(tests, "parse.decimal", parse_decimal_test);
    ut_append(tests, "parse.bignum", parse_bignum_test);
}
