// Copyright (c) 2018, Peter Ohler, All rights reserved.

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "oj.h"

#ifdef PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif

ojStatus
oj_err_set(ojErr err, int code, const char *fmt, ...) {
    va_list	ap;

    va_start(ap, fmt);
    vsnprintf(err->msg, sizeof(err->msg), fmt, ap);
    va_end(ap);
    err->code = code;

    return err->code;
}

ojStatus
oj_err_no(ojErr err, const char *fmt, ...) {
    int	cnt = 0;

    if (NULL != fmt) {
	va_list	ap;

	va_start(ap, fmt);
	cnt = vsnprintf(err->msg, sizeof(err->msg), fmt, ap);
	va_end(ap);
    }
    if (cnt < (int)sizeof(err->msg) + 2 && 0 <= cnt) {
	err->msg[cnt] = ' ';
	cnt++;
	strncpy(err->msg + cnt, strerror(errno), sizeof(err->msg) - cnt);
	err->msg[sizeof(err->msg) - 1] = '\0';
    }
    err->code = errno;

    return err->code;
}

void
oj_err_init(ojErr err) {
    err->code = OJ_OK;
    err->line = 0;
    err->col = 0;
    *err->msg = '\0';
}

const char*
oj_status_str(ojStatus code) {
    const char	*str = NULL;

    if (code < OJ_ERR_START) {
	str = strerror(code);
    }
    if (NULL == str) {
	switch (code) {
	case OJ_ERR_PARSE:	str = "parse error";		break;
	case OJ_ERR_READ:	str = "read failed";		break;
	case OJ_ERR_WRITE:	str = "write failed";		break;
	case OJ_ERR_ARG:	str = "invalid argument";	break;
	case OJ_ERR_TOO_MANY:	str = "too many";		break;
	case OJ_ERR_TYPE:	str = "type error";		break;
	case OJ_ERR_KEY:	str = "key error";		break;
	case OJ_ERR_OVERFLOW:	str = "overflow";		break;
	default:		str = "unknown error";		break;
	}
    }
    return str;
}

int
oj_err_memory(ojErr err, const char *type, const char *file, int line) {
    return oj_err_set(err, OJ_ERR_MEMORY, "Failed to allocate memory for a %s at %s:%d.", type, file, line);
}
