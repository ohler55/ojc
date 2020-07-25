// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj.h"

const char*
oj_version() {
    return OJ_VERSION;
}

const char*
oj_type_str(ojType type) {
    switch (type) {
    case OJ_NIL:	return "null";
    case OJ_TRUE:	return "true";
    case OJ_FALSE:	return "false";
    case OJ_INT:	return "int";
    case OJ_FLOAT:	return "float";
    case OJ_STRING:	return "string";
    case OJ_OBJECT:	return "object";
    case OJ_ARRAY:	return "array";
    default:		return "unknown";
    }
}

const char*
oj_status_str(ojStatus code) {
    switch (code) {
    case OJ_OK:			return "ok";
    case OJ_TYPE_ERR:		return "type error";
    case OJ_PARSE_ERR:		return "parse error";
    case OJ_INCOMPLETE_ERR:	return "incomplete parse error";
    case OJ_OVERFLOW_ERR:	return "overflow error";
    case OJ_WRITE_ERR:		return "write error";
    case OJ_MEMORY_ERR:		return "memory error";
    case OJ_UNICODE_ERR:	return "unicode error";
    case OJ_ABORT_ERR:		return "abort";
    case OJ_ARG_ERR:		return "argument error";
    default:			return "unknown";
    }
}
