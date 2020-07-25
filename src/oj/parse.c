// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj.h"

void
oj_validator(ojParser p) {
    p->push = NULL;
    p->pop = NULL;
    p->cb = NULL;
    p->ctx = NULL;
    oj_err_reset(&p->err);
}

ojStatus
oj_parse_str(ojParser p, const char *json) {

    // TBD

    return OJ_OK;
}

ojStatus
oj_parse_strp(ojParser p, const char **json) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_file(ojParser p, FILE *file) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_fd(ojParser p, int socket) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_reader(ojParser p, void *src, ojReadFunc rf) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_file_follow(ojParser p, FILE *file) {
    // TBD
    return OJ_OK;
}
