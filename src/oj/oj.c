// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include "oj.h"

const char*
oj_version() {
    return OJ_VERSION;
}

const char*
oj_type_str(ojType type) {
    switch (type) {
    case OJ_NULL:	return "null";
    case OJ_TRUE:	return "true";
    case OJ_FALSE:	return "false";
    case OJ_INT:	return "int";
    case OJ_DECIMAL:	return "decimal";
    case OJ_STRING:	return "string";
    case OJ_OBJECT:	return "object";
    case OJ_ARRAY:	return "array";
    default:		return "unknown";
    }
}
