// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// Parse a JSON string using callbacks on each document. For every document
// the callback is invoked but invoked in a thread separate from the
// parser. This is an effective way to iterate over multiple documents in a
// string where significant processing time is spent on each document. There
// is some overhead using the caller so it is best used when there is some
// processing of the documents involved.

static ojCallbackOp
callback(ojVal val, void *ctx) {
    *(int*)ctx = *(int*)ctx + 1;

    // The return value indicates to the parser what whould be done after the
    // callback. If the document (val) is no longer needed then OJ_DESTROY is
    // returned. If the parsing should be stopped then OJ_STOP is returned. To
    // both stop and cleanup the two values are ORed together (OJ_DESTROY |
    // OJ_STOP).
    return OJ_DESTROY;
}

int
main(int argc, char **argv) {
    const char		*filename = "sample.json";
    struct _ojErr	err = OJ_ERR_INIT;
    ojStatus		status;
    struct _ojCaller	caller;
    int			iter = 0;

    // Since multiple threads are being used, make sure the thread safe mode
    // it activated.
    oj_thread_safe = true;
    // Stater the caller. The caller contains the thread that will invoke the
    // callback function.
    oj_caller_start(&err, &caller, callback, &iter);

    printf("Parsing %s\n", filename);
    status = oj_parse_file_call(&err, filename, &caller);
    // Wait for the caller to finish processing.
    oj_caller_wait(&caller);
    if (OJ_OK == status) {
	printf("Parsed %d documents\n", iter);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    return err.code;
}
