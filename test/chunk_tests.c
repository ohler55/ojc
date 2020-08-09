// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "ut.h"

struct _data {
    const char	**input;
    const char	*expect;
    ojStatus	status;
};

static void
eval_data(struct _data *data) {
    int	sv[2];
    // TBD
    printf("*** input - %p\n", (void*)data);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
	if (ut_handle_errno()) {
	    return;
	}
    }
    printf("*** sv: %d %d\n", sv[0], sv[1]);

    // TBD start parse on one side
    // start thread to write

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

void
append_chunk_tests(Test tests) {
    ut_append(tests, "chunk.false", chunk_false_test);
}
