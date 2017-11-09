/*******************************************************************************
 * Copyright (c) 2014 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include "ojc/buf.h"
#include "ojc/wire.h"
#include "ut.h"

typedef struct _Jlen {
    const char	*json;
    int		len;
} *Jlen;

void
wire_size_test() {
    ojcVal		val;
    struct _ojcErr	err;
    struct _Jlen	data[] = {
	{ "null", 5 },
	{ "false", 5 },
	{ "true", 5 },
	{ "127", 6 },
	{ "128", 7 },
	{ "-40000", 9 },
	{ "\"hello\"", 11 },
	{ "12.3", 10 },
	{ "{}", 6 },
	{ "[]", 6 },
	{ "[1,2,3]", 12 },
	{ "{\"abc\":1,\"def\":2}", 20 },
	{ "{\"abc\":1,\"def\":[1,2]}", 24 },
	{ "\"123e4567-e89b-12d3-a456-426655440000\"", 21 },
	{ "\"2017-03-14T15:09:26.123456789Z\"", 13 },
	{ NULL, 0 }};

    for (Jlen jl = data; NULL != jl->json; jl++) {
	ojc_err_init(&err);
	val = ojc_parse_str(&err, jl->json, 0, 0);
	if (ut_handle_error(&err)) {
	    continue;
	}
	ut_same_int(jl->len, ojc_wire_size(val), "%s", jl->json);
    }
}

static ojcVal
build_sample() {
    struct _ojcErr	err = OJC_ERR_INIT;
    ojcVal		val = ojc_parse_str(&err, "{\n\
  \"nil\":null,\n\
  \"yes\":true,\n\
  \"no\":false,\n\
  \"int\":12345,\n\
  \"array\":[\n\
    -23,\n\
    1.23,\n\
    \"string\",\n\
    \"123e4567-e89b-12d3-a456-426655440000\",\n\
    \"2017-03-14T15:09:26.123456789Z\"\n\
  ]\n\
}", 0, 0);

    if (ut_handle_error(&err)) {
	return NULL;
    }
    return val;
}

static const char	*expect_sample_dump = "\
00 00 00 4E 7B 6B 03 6E  69 6C 5A 6B 03 79 65 73   ...N{k.n ilZk.yes\n\
74 6B 02 6E 6F 66 6B 03  69 6E 74 32 30 39 6B 05   tk.nofk. int209k.\n\
61 72 72 61 79 5B 69 E9  64 04 31 2E 32 33 73 06   array[i. d.1.23s.\n\
73 74 72 69 6E 67 75 12  3E 45 67 E8 9B 12 D3 A4   stringu. >Eg.....\n\
56 42 66 55 44 00 00 54  14 AB C8 25 B9 40 C9 15   VBfUD..T ...%.@..\n\
5D 7D                                              ]}\n";

void
wire_fill_test() {
    ojcVal	val = build_sample();

    if (NULL == val) {
	return;
    }
    uint8_t	wire[1024];
    size_t	size = ojc_wire_fill(val, wire, 0);
    char	buf[1024];

    ut_hexDumpBuf(wire, (int)size, buf);
    ut_same(expect_sample_dump, buf);
    ojc_destroy(val);
}

void
wire_mem_test() {
    ojcVal	val = build_sample();

    if (NULL == val) {
	return;
    }
    struct _ojcErr	err = OJC_ERR_INIT;
    uint8_t		*wire = ojc_wire_write_mem(&err, val);
    char		buf[1024];

    ut_hexDumpBuf(wire, (int)ojc_wire_size(val), buf);
    ut_same(expect_sample_dump, buf);
    ojc_destroy(val);
    free(wire);
}

void
wire_file_test() {
    ojcVal	val = build_sample();

    if (NULL == val) {
	return;
    }
    FILE		*f = fopen("tmp.wire", "w");
    struct _ojcErr	err = OJC_ERR_INIT;

    ojc_wire_write_file(&err, val, f);
    fclose(f);
    if (ut_handle_error(&err)) {
	return;
    }
    char	*wire = ut_loadFile("tmp.wire");
    char	buf[1024];

    ut_hexDumpBuf((uint8_t*)wire, (int)ojc_wire_size(val), buf);
    ut_same(expect_sample_dump, buf);
    ojc_destroy(val);
    free(wire);
}

static int
wire_build_sample(ojcWire wire) {
    struct _ojcErr	err = OJC_ERR_INIT;
    
    ojc_wire_push_object(&err, wire);

    ojc_wire_push_key(&err, wire, "nil", -1);
    ojc_wire_push_null(&err, wire);

    ojc_wire_push_key(&err, wire, "yes", -1);
    ojc_wire_push_bool(&err, wire, true);
    
    ojc_wire_push_key(&err, wire, "no", -1);
    ojc_wire_push_bool(&err, wire, false);

    ojc_wire_push_key(&err, wire, "int", -1);
    ojc_wire_push_int(&err, wire, 12345);

    ojc_wire_push_key(&err, wire, "array", -1);
    ojc_wire_push_array(&err, wire);
    
    ojc_wire_push_int(&err, wire, -23);
    ojc_wire_push_double(&err, wire, 1.23);
    ojc_wire_push_string(&err, wire, "string", -1);
    ojc_wire_push_uuid_string(&err, wire, "123e4567-e89b-12d3-a456-426655440000");
    ojc_wire_push_time(&err, wire, 1489504166123456789LL);

    ojc_wire_finish(&err, wire);
    if (ut_handle_error(&err)) {
	return err.code;
    }
    return OJC_OK;
}

void
wire_build_buf_test() {
    struct _ojcWire	wire;
    struct _ojcErr	err = OJC_ERR_INIT;
    uint8_t		data[1024];
    
    ojc_wire_init(&err, &wire, data, sizeof(data));
    wire_build_sample(&wire);

    char	buf[1024];

    ut_hexDumpBuf(data, (int)ojc_wire_length(&wire), buf);
    ut_same(expect_sample_dump, buf);
    ojc_wire_cleanup(&wire);
}

void
wire_build_alloc_test() {
    struct _ojcWire	wire;
    struct _ojcErr	err = OJC_ERR_INIT;
    
    ojc_wire_init(&err, &wire, NULL, 0);
    wire_build_sample(&wire);

    char	buf[1024];

    ut_hexDumpBuf(wire.buf, (int)ojc_wire_length(&wire), buf);
    ut_same(expect_sample_dump, buf);
    ojc_wire_cleanup(&wire);
}

static int
begin_object(ojcErr err, void *ctx) {
    strcat((char*)ctx, "{");
    return OJC_OK;
}

static int
end_object(ojcErr err, void *ctx) {
    strcat((char*)ctx, "}");
    return OJC_OK;
}

static int
begin_array(ojcErr err, void *ctx) {
    strcat((char*)ctx, "[");
    return OJC_OK;
}

static int
end_array(ojcErr err, void *ctx) {
    strcat((char*)ctx, "]");
    return OJC_OK;
}

static int
key(ojcErr err, const char *key, int len, void *ctx) {
    strncat((char*)ctx, key, len);
    strcat((char*)ctx, ":");
    return OJC_OK;
}

static int
null(ojcErr err, void *ctx) {
    strcat((char*)ctx, "null ");
    return OJC_OK;
}

static int
boolean(ojcErr err, bool b, void *ctx) {
    if (b) {
	strcat((char*)ctx, "true ");
    } else {
	strcat((char*)ctx, "false ");
    }
    return OJC_OK;
}

static int
fixnum(ojcErr err, int64_t num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%lld ", (unsigned long long)num);
    strcat((char*)ctx, buf);
    return OJC_OK;
}

static int
decimal(ojcErr err, double num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%f ", num);
    strcat((char*)ctx, buf);
    return OJC_OK;
}

static int
string(ojcErr err, const char *str, int len, void *ctx) {
    strncat((char*)ctx, str, len);
    strcat((char*)ctx, " ");
    return OJC_OK;
}

static int
uuid_str(ojcErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");
    return OJC_OK;
}

static int
time_str(ojcErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");

    return OJC_OK;
}

void
wire_parse_cb_test() {
    struct _ojcWire	wire;
    struct _ojcErr	err = OJC_ERR_INIT;
    uint8_t		data[1024];
    
    ojc_wire_init(&err, &wire, data, sizeof(data));
    wire_build_sample(&wire);

    char			buf[1024];
    struct _ojcWireCallbacks	callbacks = {
	.begin_object = begin_object,
	.end_object = end_object,
	.begin_array = begin_array,
	.end_array = end_array,
	.key = key,
	.null = null,
	.boolean = boolean,
	.fixnum = fixnum,
	.decimal = decimal,
	.string = string,
	.uuid_str = uuid_str,
	.time_str = time_str,
    };
    *buf = '\0';
    ojc_wire_cbparse(&err, wire.buf, &callbacks, buf);
    
    ut_same("{nil:null yes:true no:false int:12345 array:[-23 1.230000 string 123e4567-e89b-12d3-a456-426655440000 2017-03-14T15:09:26.123456789Z ]}", buf);
    ojc_wire_cleanup(&wire);
}

void
wire_parse_test() {
    struct _ojcWire	wire;
    struct _ojcErr	err = OJC_ERR_INIT;
    uint8_t		data[1024];
    
    ojc_wire_init(&err, &wire, data, sizeof(data));
    wire_build_sample(&wire);

    ojcVal	val = ojc_wire_parse(&err, wire.buf);
    struct _Buf	buf;

    buf_init(&buf, 0);
    ojc_buf(&buf, val, 2, 0);
    ut_same("{\n\
  \"nil\":null,\n\
  \"yes\":true,\n\
  \"no\":false,\n\
  \"int\":12345,\n\
  \"array\":[\n\
    -23,\n\
    1.23,\n\
    \"string\",\n\
    \"123e4567-e89b-12d3-a456-426655440000\",\n\
    \"2017-03-14T15:09:26.123456789Z\"\n\
  ]\n\
}", buf.head);
    ojc_wire_cleanup(&wire);
}


