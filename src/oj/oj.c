// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "oj.h"
#include "buf.h"
#include "intern.h"

typedef struct _Esc {
    int		len;
    const char	*seq;
} *Esc;

static struct _Esc	esc_map[0x20] = {
    { .len = 6, .seq = "\\u0000" },
    { .len = 6, .seq = "\\u0001" },
    { .len = 6, .seq = "\\u0002" },
    { .len = 6, .seq = "\\u0003" },
    { .len = 6, .seq = "\\u0004" },
    { .len = 6, .seq = "\\u0005" },
    { .len = 6, .seq = "\\u0006" },
    { .len = 6, .seq = "\\u0007" },
    { .len = 2, .seq = "\\b" },
    { .len = 2, .seq = "\\t" },
    { .len = 2, .seq = "\\n" },
    { .len = 6, .seq = "\\u000b" },
    { .len = 2, .seq = "\\f" },
    { .len = 2, .seq = "\\r" },
    { .len = 6, .seq = "\\u000e" },
    { .len = 6, .seq = "\\u000f" },
    { .len = 6, .seq = "\\u0010" },
    { .len = 6, .seq = "\\u0011" },
    { .len = 6, .seq = "\\u0012" },
    { .len = 6, .seq = "\\u0013" },
    { .len = 6, .seq = "\\u0014" },
    { .len = 6, .seq = "\\u0015" },
    { .len = 6, .seq = "\\u0016" },
    { .len = 6, .seq = "\\u0017" },
    { .len = 6, .seq = "\\u0018" },
    { .len = 6, .seq = "\\u0019" },
    { .len = 6, .seq = "\\u001a" },
    { .len = 6, .seq = "\\u001b" },
    { .len = 6, .seq = "\\u001c" },
    { .len = 6, .seq = "\\u001d" },
    { .len = 6, .seq = "\\u001e" },
    { .len = 6, .seq = "\\u001f" },
};

static const char	spaces[258] = "\n                                                                                                                                                                                                                                                                ";

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
    case OJ_BIG:	return "bignum";
    case OJ_STRING:	return "string";
    case OJ_OBJECT:	return "object";
    case OJ_ARRAY:	return "array";
    default:		return "unknown";
    }
}

static void
buf_append_json(ojBuf buf, const char *s) {
    // TBD might be faster moving forward until a special char and then appending the string up till then
    for (; '\0' != *s; s++) {
	if ((byte)*s < 0x20) {
	    Esc	esc = esc_map + (byte)*s;
	    oj_buf_append_string(buf, esc->seq, esc->len);
	} else if ('"' == *s || '\\' == *s) {
	    oj_buf_append(buf, '\\');
	    oj_buf_append(buf, *s);
	} else {
	    oj_buf_append(buf, *s);
	}
    }
}

size_t
oj_buf(ojBuf buf, ojVal val, int indent, int depth) {
    size_t	start = oj_buf_len(buf);

    if (NULL != val) {
	switch (val->type) {
	case OJ_NULL:
	    oj_buf_append_string(buf, "null", 4);
	    break;
	case OJ_TRUE:
	    oj_buf_append_string(buf, "true", 4);
	    break;
	case OJ_FALSE:
	    oj_buf_append_string(buf, "false", 5);
	    break;
	case OJ_INT:
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%lld", (long long)val->num.fixnum);
	    }
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    break;
	case OJ_DECIMAL:
	    if (0 == val->num.len) {
		val->num.len = snprintf(val->num.raw, sizeof(val->num.raw), "%Lg", val->num.dub);
	    }
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    break;
	case OJ_BIG:
	    oj_buf_append_string(buf, val->num.raw, (size_t)val->num.len);
	    break;
	case OJ_STRING: {
	    const char	*s;

	    if (sizeof(union _ojS4k) < val->str.len) {
		s = val->str.ptr;
	    } else if (sizeof(val->str.raw) < val->str.len) {
		s = val->str.s4k->str;
	    } else {
		s = val->str.raw;
	    }
	    oj_buf_append(buf, '"');
	    buf_append_json(buf, s);
	    oj_buf_append(buf, '"');
	    break;
	}
	case OJ_OBJECT:
	    oj_buf_append(buf, '{');
	    if (0 < indent) {
		int	d2 = depth + 1;
		int	i = indent * depth + 1;
		int	i2 = i + 2;
		ojVal	v;
		bool	first = true;

		if (sizeof(spaces) <= i2) {
		    i2 = sizeof(spaces) - 1;
		    if (sizeof(spaces) <= i) {
			i = sizeof(spaces) - 1;
		    }
		}
		if (OJ_OBJ_HASH == val->mod) {
		    ojVal	*bucket = val->hash;
		    ojVal	*bend = bucket + sizeof(val->hash) / sizeof(*val->hash);

		    oj_buf_append(buf, '{');
		    for (; bucket < bend; bucket++) {
			for (v = *bucket; NULL != v; v = v->next) {
			    if (first) {
				first = false;
			    } else {
				oj_buf_append(buf, ',');
			    }
			    oj_buf_append_string(buf, spaces, i2);
			    oj_buf_append(buf, '"');
			    oj_buf_append_string(buf, oj_key(v), v->key.len);
			    oj_buf_append(buf, '"');
			    oj_buf_append(buf, ':');
			    oj_buf(buf, v, indent, d2);
			}
		    }
		    oj_buf_append(buf, '}');
		} else {
		    for (v = val->list.head; NULL != v; v = v->next) {
			if (first) {
			    first = false;
			} else {
			    oj_buf_append(buf, ',');
			}
			oj_buf_append_string(buf, spaces, i2);
			oj_buf_append(buf, '"');
			oj_buf_append_string(buf, oj_key(v), v->key.len);
			oj_buf_append(buf, '"');
			oj_buf_append(buf, ':');
			oj_buf(buf, v, indent, d2);
		    }
		}
		oj_buf_append_string(buf, spaces, i);
	    } else {
		ojVal	v;
		bool	first = true;

		if (OJ_OBJ_HASH == val->mod) {
		    ojVal	*bucket = val->hash;
		    ojVal	*bend = bucket + sizeof(val->hash) / sizeof(*val->hash);

		    for (; bucket < bend; bucket++) {
			for (v = *bucket; NULL != v; v = v->next) {
			    if (first) {
				first = false;
			    } else {
				oj_buf_append(buf, ',');
			    }
			    oj_buf_append(buf, '"');
			    oj_buf_append_string(buf, oj_key(v), v->key.len);
			    oj_buf_append(buf, '"');
			    oj_buf_append(buf, ':');
			    oj_buf(buf, v, 0, 0);
			}
		    }
		} else {
		    for (v = val->list.head; NULL != v; v = v->next) {
			if (first) {
			    first = false;
			} else {
			    oj_buf_append(buf, ',');
			}
			oj_buf_append(buf, '"');
			oj_buf_append_string(buf, oj_key(v), v->key.len);
			oj_buf_append(buf, '"');
			oj_buf_append(buf, ':');
			oj_buf(buf, v, 0, 0);
		    }
		}
	    }
	    oj_buf_append(buf, '}');
	    break;
	case OJ_ARRAY:
	    oj_buf_append(buf, '[');
	    if (0 < indent) {
		int	d2 = depth + 1;
		int	i = indent * depth + 1;
		int	i2 = i + 2;
		bool	first = true;

		if (sizeof(spaces) <= i2) {
		    i2 = sizeof(spaces) - 1;
		    if (sizeof(spaces) <= i) {
			i = sizeof(spaces) - 1;
		    }
		}
		for (ojVal v = val->list.head; NULL != v; v = v->next) {
		    if (first) {
			first = false;
		    } else {
			oj_buf_append(buf, ',');
		    }
		    oj_buf_append_string(buf, spaces, i2);
		    oj_buf(buf, v, indent, d2);
		}
		oj_buf_append_string(buf, spaces, i);
	    } else {
		bool	first = true;

		for (ojVal v = val->list.head; NULL != v; v = v->next) {
		    if (first) {
			first = false;
		    } else {
			oj_buf_append(buf, ',');
		    }
		    oj_buf(buf, v, 0, 0);
		}
	    }
	    oj_buf_append(buf, ']');
	    break;
	default:
	    oj_buf_append_string(buf, "??", 2);
	    break;
	}
    }
    return oj_buf_len(buf) - start;
}

char*
oj_to_str(ojVal val, int indent) {
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    oj_buf(&buf, val, indent, 0);
    oj_buf_append(&buf, '\0');
    if (buf.base == buf.head) {
	return strdup(buf.head);
    }
    return buf.head;
}

size_t
oj_fill(ojErr err, ojVal val, int indent, char *out, int max) {
    struct _ojBuf	buf;

    oj_buf_finit(&buf, out, max - 1);
    oj_buf(&buf, val, indent, 0);
    oj_buf_append(&buf, '\0');

    return oj_buf_len(&buf);
}

size_t
oj_write(ojErr err, ojVal val, int indent, int fd) {
    struct _ojBuf	buf;

    oj_buf_init(&buf, fd);
    oj_buf(&buf, val, indent, 0);
    oj_buf_finish(&buf);

    return oj_buf_len(&buf);
}

size_t
oj_fwrite(ojErr err, ojVal val, int indent, const char *filepath) {
    int	fd = open(filepath, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd < 0) {
	if (NULL != err) {
	    oj_err_no(err, "error opening %s", filepath);
	}
	return -1;
    }
    size_t	len = oj_write(err, val, indent, fd);

    close(fd);
    if (OJ_OK != err->code) {
	return -1;
    }
    return len;
}
