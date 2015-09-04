/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "ut.h"
#include "ojc.h"

static const char	bench_json[] = "{\"a\":\"Alpha\",\"b\":true,\"c\":12345,\"d\":[true,[false,[-123456789,null],3.9676,[\"Something else.\",false],null]],\"e\":{\"zero\":null,\"one\":1,\"two\":2,\"three\":[3],\"four\":[0,1,2,3,4]},\"f\":null,\"h\":{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":{\"f\":{\"g\":null}}}}}}},\"i\":[[[[[[[null]]]]]]]}";


static void
in_and_out(const char **jsons) {
    char		result[256];
    ojcVal		val;
    struct _ojcErr	err;
    const char		**jp;

    for (jp = jsons; 0 != *jp; jp++) {
	ojc_err_init(&err);
	val = ojc_parse_str(&err, *jp, 0, 0);
	if (ut_handle_error(&err)) {
	    return;
	}
	ojc_fill(&err, val, 0, result, sizeof(result) - 1);
	ut_same(*jp, result);
	ojc_destroy(val);
    }
}

static void
array_test() {
    const char	*jsons[] = {
	"[]",
	"[[]]",
	"[[],[]]",
	"[[[]]]",
	0 };

    in_and_out(jsons);
}

static void
null_test() {
    const char	*jsons[] = {
	"null",
	"[null]",
	"[null,null]",
	"[null,[],null]",
	0 };

    in_and_out(jsons);
}

static void
true_test() {
    const char	*jsons[] = {
	"true",
	"[true]",
	"[true,true]",
	"[true,[],true]",
	0 };

    in_and_out(jsons);
}

static void
false_test() {
    const char	*jsons[] = {
	"false",
	"[false]",
	"[false,false]",
	"[false,[],false]",
	0 };

    in_and_out(jsons);
}

static void
string_test() {
    const char	*jsons[] = {
	"\"hello\"",
	"\"This is a longer string.\"",
	"\"hello\\nthere\"",
	"\"ぴーたー\"",
	"[\"hello\"]",
	0 };
    const char	*newline_jsons[] = {
	"\"hello\nthere\"",
	0 };

    in_and_out(jsons);
    ojc_newline_ok = true;
    in_and_out(newline_jsons);
    ojc_newline_ok = false;
}

static void
word_test() {
    const char	*jsons[] = {
	"X",
	"Foo",
	"a_long_15_word_",
	0 };
    ojc_word_ok = true;
    in_and_out(jsons);
    ojc_word_ok = false;
}

static void
number_test() {
    const char	*jsons[] = {
	"0",
	"123",
	"-123",
	"9223372036854775807",
	"-9223372036854775807",
	"1.23",
	"-1.00023",
	"1.00000123e-08",
	"123456789000987654321",
	0 };

    in_and_out(jsons);
}

static void
opaque_test() {
    char		result[64];
    char		expect[64];
    const char		*opaque = "OjC";
    ojcVal		val = ojc_create_opaque((void*)opaque);
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*s = (const char*)ojc_opaque(&err, val);

    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(opaque, s);
    ojc_fill(&err, val, 0, result, sizeof(result) - 1);
    ut_same("", result);
    ojc_write_opaque = true;
    sprintf(expect, "%llu", (unsigned long long)opaque);
    ojc_fill(&err, val, 0, result, sizeof(result) - 1);
    ut_same(expect, result);
    ojc_write_opaque = false;

    ojc_destroy(val);
}

static void
object_test() {
    const char	*jsons[] = {
	"{}",
	"{\"one\":null}",
	"{\"one\":null,\"a key longer than sixteen characters\":true}",
	"{\"one\":{}}",
	"{\"o\\ne\":true}",
	0 };

    in_and_out(jsons);
}

static void
mix_test() {
    const char	*jsons[] = {
	"{\"a\":\"Alpha\",\"b\":true,\"c\":12345,\"d\":[true,[false,[-123456789,null],3.9676,[\"Something else.\",false],null]],\"e\":{\"zero\":null,\"one\":1,\"two\":2,\"three\":[3],\"four\":[0,1,2,3,4]},\"f\":null,\"h\":{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":{\"f\":{\"g\":null}}}}}}},\"i\":[[[[[[[null]]]]]]]}",
	0 };

    in_and_out(jsons);
}

static void
comment_test() {
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\n\
  // a comment\n\
  \"a\":\"Alpha\",\n\
  // another comment\n\
  \"b\":true,\n\
  /* third comment */\n\
  \"c\":12345\n\
}";
    char		result[256];

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fill(&err, val, 0, result, sizeof(result) - 1);
    ut_same("{\"a\":\"Alpha\",\"b\":true,\"c\":12345}", result);
    ojc_destroy(val);
}

static bool
each_callback(ojcErr err, ojcVal val, void *ctx) {
    char		*result = (char*)ctx;
    int			cnt;

    for (; '\0' != *result; result++) {
    }
    cnt = ojc_fill(err, val, 0, result, 32);
    result[cnt] = '\n';
    result[cnt + 1] = '\0';

    return true;
}

static void
each_test() {
    const char	*json = "\n\
null\n\
{}\n\
[]\n\
\"hello\"\n\
";
    char		result[256];
    struct _ojcErr	err;

    ojc_err_init(&err);
    *result = '\n';
    result[1] = '\0';
    ojc_parse_str(&err, json, each_callback, result);
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(json, result);
}

static void
file_parse_test() {
    FILE		*f = fopen("tmp.json", "w");
    char		result[300];
    struct _ojcErr	err;
    ojcVal		val;

    fwrite(bench_json, sizeof(bench_json) - 1, 1, f);
    fclose(f);

    ojc_err_init(&err);
    f = fopen("tmp.json", "r");
    val = ojc_parse_stream(&err, f, 0, 0);
    fclose(f);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fill(&err, val, 0, result, sizeof(result));

    ut_same(bench_json, result);
}

static const char	*follow_json2 = "[3,2,1]";

static bool
follow_callback(ojcErr err, ojcVal val, void *ctx) {
    ojcVal	*vp = (ojcVal*)ctx;
    
    if (0 == *vp) {
	FILE		*f = fopen("tmp.json", "a");
	
	// write string and terminating \0
	fwrite(follow_json2, 1, strlen(follow_json2) + 1, f);
	fclose(f);
    }
    *vp = val;

    return false;
}

static void
follow_parse_test() {
    FILE		*f = fopen("tmp.json", "w");
    char		result[300];
    struct _ojcErr	err;
    ojcVal		val = NULL;
    const char		*json = "[1,2,3,4]";

    fwrite(json, 1, strlen(json), f);
    fclose(f);

    ojc_err_init(&err);
    f = fopen("tmp.json", "r");
    ojc_parse_stream_follow(&err, f, follow_callback, &val);
    fclose(f);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fill(&err, val, 0, result, sizeof(result));

    ut_same(follow_json2, result);
}

static ssize_t
my_read_func(void *src, char *buf, size_t size) {
    return fread(buf, 1, size, (FILE*)src);
}

static void
func_parse_test() {
    FILE		*f = fopen("tmp.json", "w");
    char		result[300];
    struct _ojcErr	err;
    ojcVal		val;

    fwrite(bench_json, sizeof(bench_json) - 1, 1, f);
    fclose(f);

    ojc_err_init(&err);
    f = fopen("tmp.json", "r");
    val = ojc_parse_reader(&err, f, my_read_func, 0, 0);
    fclose(f);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fill(&err, val, 0, result, sizeof(result));

    ut_same(bench_json, result);
}


static void
file_write_test() {
    FILE		*f = fopen("tmp.json", "w");
    char		*result;
    struct _ojcErr	err;
    ojcVal		val;

    ojc_err_init(&err);
    val = ojc_parse_str(&err, bench_json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fwrite(&err, val, 0, f);
    fclose(f);

    result = ut_loadFile("tmp.json");
    ut_same(bench_json, result);

    free(result);
}

static void
fill_too_big_test() {
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "[0,1,2,3,4,5,6,7,8,9]";
    char		result[16];

    memset(result, '\0', sizeof(result));
    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_fill(&err, val, 0, result, 10);
    ut_same_int(OJC_OVERFLOW_ERR, err.code, "overflow check");
    ojc_destroy(val);
}

typedef struct _PathExp {
    const char	*path;
    const char	*expect;
} *PathExp;

static void
array_get_test() {
    char		result[256];
    ojcVal		array;
    ojcVal		val;
    struct _ojcErr	err;
    PathExp		pe;
    const char		*json = "[1,[[\"word\",false],null,true],3,4,5,6,7,8,9,10,11,12,13,14]";
    struct _PathExp	data[] = {
	{ "0",		"1" },
	{ 0,		json },
	{ "1",		"[[\"word\",false],null,true]" },
	{ "2",		"3" },
	{ "12",		"13" },
	{ "13",		"14" },
	{ "14",		"" },
	{ "1/0",	"[\"word\",false]" },
	{ "1.0.1",	"false" },
	{ "/1/0/1",	"false" },
	{ "/1/0.2",	"" },
	{ "/10",	"11" },
	{ "foo",	"" },
	{ 0, 0 } };

    ojc_err_init(&err);
    array = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    for (pe = data; 0 != pe->expect; pe++) {
	val = ojc_get(array, pe->path);
	ojc_fill(&err, val, 0, result, sizeof(result));
	ut_same(pe->expect, result);
    }
}

static void
object_get_test() {
    char		result[256];
    ojcVal		array;
    ojcVal		val;
    struct _ojcErr	err;
    PathExp		pe;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":false},\"two\":12345}";
    struct _PathExp	data[] = {
	{ "zero",	"0" },
	{ "one",	"{\"1\":true,\"2\":null,\"3\":false}" },
	{ "two",	"12345" },
	{ "/two",	"12345" },
	{ "three",	"" },
	{ "one/1",	"true" },
	{ "one.2",	"null" },
	{ "one/3",	"false" },
	{ 0, 0 } };

    ojc_err_init(&err);
    array = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    for (pe = data; 0 != pe->expect; pe++) {
	val = ojc_get(array, pe->path);
	ojc_fill(&err, val, 0, result, sizeof(result));
	ut_same(pe->expect, result);
    }
}

static void
object_get_size_test() {
    char		result[256];
    ojcVal		array;
    ojcVal		val;
    struct _ojcErr	err;
    PathExp		pe;
    const char		*json = "{\"zerozero\":1,\"zerozer\":0,\"zerozeroz\":2}";
    struct _PathExp	data[] = {
	{ "zerozer",	"0" },
	{ "zerozero",	"1" },
	{ "zerozeroz",	"2" },
	{ 0, 0 } };

    ojc_err_init(&err);
    array = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    for (pe = data; 0 != pe->expect; pe++) {
	val = ojc_get(array, pe->path);
	ojc_fill(&err, val, 0, result, sizeof(result));
	ut_same(pe->expect, result);
    }
}

typedef struct _APathExp {
    const char	*path[4];
    const char	*expect;
} *APathExp;

static void
array_aget_test() {
    char		result[256];
    ojcVal		array;
    ojcVal		val;
    struct _ojcErr	err;
    APathExp		pe;
    const char		*json = "[1,[[\"word\",false],null,true],3,4,5,6,7,8,9,10,11,12,13,14]";
    struct _APathExp	data[] = {
	{ {"0",0},		"1" },
	{ {"1",0},		"[[\"word\",false],null,true]" },
	{ {"2",0},		"3" },
	{ {"12",0},		"13" },
	{ {"13",0},		"14" },
	{ {"14",0},		"" },
	{ {"1","0",0},		"[\"word\",false]" },
	{ {"1","0","1",0},	"false" },
	{ {"1","0","2",0},	"" },
	{ {"10",0},		"11" },
	{ {"foo",0},		"" },
	{ {0}, 0 } };

    ojc_err_init(&err);
    array = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    for (pe = data; 0 != pe->expect; pe++) {
	val = ojc_aget(array, pe->path);
	ojc_fill(&err, val, 0, result, sizeof(result));
	ut_same(pe->expect, result);
    }
}

static void
object_aget_test() {
    char		result[256];
    ojcVal		array;
    ojcVal		val;
    struct _ojcErr	err;
    APathExp		pe;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":false},\"two\":12345}";
    struct _APathExp	data[] = {
	{ {"zero",0},		"0" },
	{ {"one",0},		"{\"1\":true,\"2\":null,\"3\":false}" },
	{ {"two",0},		"12345" },
	{ {"three",0},		"" },
	{ {"one","1",0},	"true" },
	{ {"one","2",0},	"null" },
	{ {"one","3",0},	"false" },
	{ {0}, 0 } };

    ojc_err_init(&err);
    array = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    for (pe = data; 0 != pe->expect; pe++) {
	val = ojc_aget(array, pe->path);
	ojc_fill(&err, val, 0, result, sizeof(result));
	ut_same(pe->expect, result);
    }
}

static void
duplicate_test() {
    char		expect[256];
    char		actual[256];
    ojcVal		src;
    ojcVal		dup;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":[1,2,3]},\"two\":12345}";

    src = ojc_parse_str(&err, json, 0, 0);
    ojc_fill(&err, src, 0, expect, sizeof(expect));
    if (ut_handle_error(&err)) {
	return;
    }
    dup = ojc_duplicate(src);
    ojc_fill(&err, dup, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
replace_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":[1,2,3]},\"two\":12345}";
    const char		*expect = "{\"zero\":0,\"one\":{\"1\":false,\"2\":null,\"3\":[true,2,3],\"4\":{\"one\":141}},\"two\":2,\"three\":3}";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_replace(&err, val, "two", ojc_create_int(2));
    ojc_replace(&err, val, "three", ojc_create_int(3));
    ojc_replace(&err, val, "one/1", ojc_create_bool(false));
    ojc_replace(&err, val, "one/3/0", ojc_create_bool(true));
    ojc_replace(&err, val, "one/4/one", ojc_create_int(141));
    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
array_replace_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "[0,1,2,3,4]";
    const char		*expect = "[0,-1,2,3,-4]";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_array_replace(&err, val, -1, ojc_create_int(-4));
    ojc_array_replace(&err, val, 1, ojc_create_int(-1));
    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
areplace_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":[1,2,3]},\"two\":12345}";
    const char		*expect = "{\"zero\":0,\"one\":{\"1\":false,\"2\":null,\"3\":true},\"two\":2,\"three\":3}";
    const char		*path[3] = { 0, 0, 0 };

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    path[0] = "two";
    ojc_areplace(&err, val, path, ojc_create_int(2));
    path[0] = "three";
    ojc_areplace(&err, val, path, ojc_create_int(3));
    path[0] = "one";
    path[1] = "1";
    ojc_areplace(&err, val, path, ojc_create_bool(false));
    path[1] = "3";
    ojc_areplace(&err, val, path, ojc_create_bool(true));
    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
append_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null},\"two\":2}";
    const char		*expect = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":true,\"3\":false},\"two\":2,\"three\":3}";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_append(&err, val, "three", ojc_create_int(3));
    ojc_append(&err, val, "one/3", ojc_create_bool(true));
    ojc_append(&err, val, "one/3", ojc_create_bool(false));
    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
aappend_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null},\"two\":2}";
    const char		*expect = "{\"zero\":0,\"one\":{\"1\":true,\"2\":null,\"3\":true,\"3\":false},\"two\":2,\"three\":3}";
    const char		*path[3] = { 0, 0, 0 };

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    path[0] = "three";
    ojc_aappend(&err, val, path, ojc_create_int(3));
    path[0] = "one";
    path[1] = "3";
    ojc_aappend(&err, val, path, ojc_create_bool(true));
    ojc_aappend(&err, val, path, ojc_create_bool(false));
    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

static void
object_remove_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "{\"zero\":0,\"one\":1,\"two\":2,\"three\":3,\"four\":4}";
    const char		*expect = "{\"zero\":0,\"two\":2,\"three\":3}";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_remove_by_pos(&err, val, -1);
    ojc_remove_by_pos(&err, val, 1);

    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);

    ojc_remove_by_pos(&err, val, 4);
    ut_same_int(OJC_ARG_ERR, err.code, "expected an error code");
}

static void
array_remove_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "[0,1,2,3,4]";
    const char		*expect = "[0,2,3]";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_remove_by_pos(&err, val, -1);
    ojc_remove_by_pos(&err, val, 1);

    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);

    ojc_remove_by_pos(&err, val, 4);
    ut_same_int(OJC_ARG_ERR, err.code, "expected an error code");
}

static void
array_insert_test() {
    char		actual[256];
    ojcVal		val;
    struct _ojcErr	err = OJC_ERR_INIT;
    const char		*json = "[2,3]";
    const char		*expect = "[0,1,2,3,4]";

    val = ojc_parse_str(&err, json, 0, 0);
    if (ut_handle_error(&err)) {
	return;
    }
    ojc_array_insert(&err, val, -1, ojc_create_int(4));
    ojc_array_insert(&err, val, 0, ojc_create_int(0));
    ojc_array_insert(&err, val, 1, ojc_create_int(1));

    ojc_fill(&err, val, 0, actual, sizeof(actual));
    if (ut_handle_error(&err)) {
	return;
    }
    ut_same(expect, actual);
}

typedef struct _CmpPair {
    const char	*json1;
    const char	*json2;
    int		result;
} *CmpPair;

static void
cmp_test() {
    ojcVal		v1;
    ojcVal		v2;
    struct _ojcErr	err;
    CmpPair		cp;
    struct _CmpPair	data[] = {
	{ "null", "null", 0 },
	{ "true", "false", 14 },
	{ "true", "true", 0 },
	{ "false", "false", 0 },
	{ "7", "7", 0 },
	{ "6", "7", -1 },
	{ "7", "6", 1 },
	{ "7.1", "7.0", 1 },
	{ "7.0", "7.1", -1 },
	{ "7.1", "7.1", 0 },
	{ "\"abc\"", "\"abc\"", 0 },
	{ "\"abc\"", "\"\"", 97 },
	{ "\"ab\"", "\"abc\"", -99 },
	{ "\"abc\"", "\"abd\"", -1 },
	{ "abc", "abc", 0 },
	{ "ab", "abc", -99 },
	{ "abd", "abc", 1 },
	{ "[]", "[]", 0 },
	{ "[1]", "[1]", 0 },
	{ "[1,2]", "[1]", 1 },
	{ "[1,2]", "[1,[]]", 13 },
	{ "{}", "{}", 0 },
	{ "{\"a\":1}", "{\"a\":1}", 0 },
	{ "{\"a\":1}", "{\"b\":1}", -1 },
	{ "{\"a\":2}", "{\"a\":1}", 1 },
	{ "null", "true", -116 },
	{ "true", "null", 116 },
	{ "1", "1.0", 10 },
	{ 0, 0, 0 }};

    ojc_word_ok = true;
    for (cp = data; 0 != cp->json1; cp++) {
	ojc_err_init(&err);
	v1 = ojc_parse_str(&err, cp->json1, 0, 0);
	v2 = ojc_parse_str(&err, cp->json2, 0, 0);
	if (ut_handle_error(&err)) {
	    continue;
	}
	// TBD show which fail
	ut_same_int(cp->result, ojc_cmp(v1, v2), "ojc_cmp(%s, %s)", cp->json1, cp->json2);
    }
}

static void
bench(int64_t iter, void *ctx) {
    struct _ojcErr	err;
    ojcVal		val;

    ojc_err_init(&err);
    for (; 0 < iter; iter--) {
	val = ojc_parse_str(&err, (const char*)ctx, 0, 0);
	ojc_destroy(val);
    }
}

static void
benchmark_test() {
    ut_benchmark("one at a time", 100000LL, bench, (void*)bench_json);
}

static bool
each_benchmark_callback(ojcErr err, ojcVal val, void *ctx) {
    return true;
}

static void
each_bench(int64_t iter, void *ctx) {
    struct _ojcErr	err;

    ojc_err_init(&err);
    iter /= 1000LL;
    for (; 0 < iter; iter--) {
	ojc_parse_str(&err, (const char*)ctx, each_benchmark_callback, 0);
    }
}

static void
each_benchmark_test() {
    int		cnt = 1000;
    char	*json = (char*)malloc(sizeof(bench_json) * cnt + 1);
    char	*s = json;

    for (; 0 < cnt; cnt--) {
	memcpy(s, bench_json, sizeof(bench_json));
	s += sizeof(bench_json) - 1;
	*s++ = '\n';
    }
    *s = '\0';
    ut_benchmark("each callback", 100000LL, each_bench, json);
}

static bool
free_benchmark_callback(ojcErr err, ojcVal val, void *ctx) {
    ojc_destroy(val);
    return false;
}

static void
free_bench(int64_t iter, void *ctx) {
    struct _ojcErr	err;

    ojc_err_init(&err);
    iter /= 1000LL;
    for (; 0 < iter; iter--) {
	ojc_parse_str(&err, (const char*)ctx, free_benchmark_callback, 0);
    }
}

static void
free_benchmark_test() {
    int		cnt = 1000;
    char	*json = (char*)malloc(sizeof(bench_json) * cnt + 1);
    char	*s = json;

    for (; 0 < cnt; cnt--) {
	memcpy(s, bench_json, sizeof(bench_json));
	s += sizeof(bench_json) - 1;
	*s++ = '\n';
    }
    *s = '\0';
    ut_benchmark("free callback", 100000LL, free_bench, json);
}

static void
each_str255_benchmark_test() {
    int		cnt = 1000;
    char	js[] = "\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"";
    char	*json = (char*)malloc(sizeof(js) * cnt + 1);
    char	*s = json;

    for (; 0 < cnt; cnt--) {
	memcpy(s, js, sizeof(js));
	s += sizeof(js) - 1;
	*s++ = '\n';
    }
    *s = '\0';
    ut_benchmark("each str255 callback", 100000LL, each_bench, json);
}

static void
each_str257_benchmark_test() {
    int		cnt = 1000;
    char	js[] = "\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxzz\"";
    char	*json = (char*)malloc(sizeof(js) * cnt + 1);
    char	*s = json;

    for (; 0 < cnt; cnt--) {
	memcpy(s, js, sizeof(js));
	s += sizeof(js) - 1;
	*s++ = '\n';
    }
    *s = '\0';
    ut_benchmark("each str257 callback", 100000LL, each_bench, json);
}

static struct _Test	tests[] = {
    { "array",		array_test },
    { "null",		null_test },
    { "true",		true_test },
    { "false",		false_test },
    { "string",		string_test },
    { "word",		word_test },
    { "number",		number_test },
    { "opaque",		opaque_test },
    { "object",		object_test },
    { "mix",		mix_test },
    { "comment",	comment_test },
    { "each",		each_test },
    { "file_parse",	file_parse_test },
    { "follow_parse",	follow_parse_test },
    { "func_parse",	func_parse_test },
    { "file_write",	file_write_test },
    { "fill_too_big",	fill_too_big_test },
    { "array_get",	array_get_test },
    { "object_get",	object_get_test },
    { "object_get_size",object_get_size_test },
    { "array_aget",	array_aget_test },
    { "object_aget",	object_aget_test },
    { "duplicate",	duplicate_test },
    { "replace",	replace_test },
    { "array_replace",	array_replace_test },
    { "areplace",	areplace_test },
    { "append",		append_test },
    { "aappend",	aappend_test },
    { "object_remove",	object_remove_test },
    { "array_remove",	array_remove_test },
    { "array_insert",	array_insert_test },
    { "cmp",		cmp_test },

    { "benchmark",	benchmark_test },
    { "each_benchmark",	each_benchmark_test },
    { "free_benchmark",	free_benchmark_test },
    { "each_str255_benchmark",	each_str255_benchmark_test },
    { "each_str257_benchmark",	each_str257_benchmark_test },
    { 0, 0 } };

int
main(int argc, char **argv) {
    ut_init(argc, argv, "ojc", tests);

    ut_done();

    return 0;
}
