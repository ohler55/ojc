// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj.h"

void
oj_val_parser(ojParser p) {
    p->push = NULL; // TBD set to val builder function
    p->pop = NULL; // TBD set to val builder function
    p->cb = NULL; // TBD set callback to set final result
    p->cb_ctx = NULL; // TBD set to what ever is needed
    p->pp_ctx = NULL; // TBD set to what ever is needed
    oj_err_init(&p->err);
}

ojVal
oj_val_parse_str(ojErr err, const char *json) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_strp(ojErr err, const char **json) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_file(ojErr err, FILE *file) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_fd(ojErr err, int socket) {

    // TBD

    return NULL;
}

ojVal
oj_val_parse_reader(ojErr err, void *src, ojReadFunc rf) {

    // TBD

    return NULL;
}
