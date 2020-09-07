// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "ut.h"

struct _data {
    const char	**input;
    const char	*expect;
    ojStatus	status;
    int		wd;
};

static void*
write_chunks(void *ctx) {
    struct _data	*data = (struct _data*)ctx;
    const char		**sp;

    for (sp = data->input; NULL != *sp; sp++) {
	if (write(data->wd, *sp, strlen(*sp)) < 0) {
	    break;
	}
	usleep(100);
    }
    close(data->wd);

    return NULL;
}

static void
eval_data(struct _data *data) {
    int	sv[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
	if (ut_handle_errno()) {
	    return;
	}
    }
    data->wd = sv[0];
    pthread_t	t;
    pthread_create(&t, NULL, write_chunks, data);
    pthread_detach(t);

    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val = oj_parse_fd(&err, sv[1], NULL);

    close(sv[1]);
    if (ut_handle_oj_error(&err)) {
	ut_print("error at %d:%d\n",  err.line, err.col);
	return;
    }
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    oj_buf(&buf, val, 0, 0);

    ut_same(data->expect, buf.head);
    oj_buf_cleanup(&buf);
    oj_destroy(val);
}

static void
chunk_null_test() {
    const char		*input[] = {"[nul", "l,nu", "ll,n", "ull]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[null,null,null]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

static void
chunk_false_test() {
    const char		*input[] = {"[fals", "e,fal", "se,fa", "lse,f", "alse]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[false,false,false,false]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

static void
chunk_true_test() {
    const char		*input[] = {"[tru", "e,tr", "ue,t", "rue]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[true,true,true]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

static void
chunk_string_test() {
    const char		*input[] = {"[\"ab", "c\",\"", "def\",\"ghi", "\"]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[\"abc\",\"def\",\"ghi\"]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

static void
chunk_int_test() {
    const char		*input[] = {"[1", "23,", "123", ",-", "123", ",123456789012345678901]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[123,123,-123,123456789012345678901]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

static void
chunk_decimal_test() {
    const char		*input[] = {"[1.", "23,1", ".23e", "3,-", "1.23", "e3]", NULL};
    struct _data	data = {
	.input = input,
	.expect = "[1.23,1230,-1230]",
	.status = OJ_OK,
    };
    eval_data(&data);
}

void
append_chunk_tests(Test tests) {
    ut_append(tests, "chunk.null", chunk_null_test);
    ut_append(tests, "chunk.false", chunk_false_test);
    ut_append(tests, "chunk.true", chunk_true_test);
    ut_append(tests, "chunk.string", chunk_string_test);
    ut_append(tests, "chunk.int", chunk_int_test);
    ut_append(tests, "chunk.decimal", chunk_decimal_test);
}
