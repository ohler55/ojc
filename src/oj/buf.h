// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_BUF_H
#define OJ_BUF_H

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

inline static void
oj_buf_init(ojBuf buf, int fd) {
    buf->head = buf->base;
    buf->end = buf->base + sizeof(buf->base) - 1;
    buf->tail = buf->head;
    buf->fd = fd;
    buf->realloc_ok = (0 == fd);
    *buf->head = '\0';
    buf->err = OJ_OK;
}

inline static void
oj_buf_finit(ojBuf buf, char *str, size_t slen) {
    buf->head = str;
    buf->end = str + slen;
    buf->tail = buf->head;
    buf->fd = 0;
    buf->realloc_ok = false;
    buf->err = OJ_OK;
}

inline static void
oj_buf_reset(ojBuf buf) {
    buf->head = buf->base;
    buf->tail = buf->head;
    *buf->head = '\0';
    buf->err = OJ_OK;
}

inline static void
oj_buf_cleanup(ojBuf buf) {
    if (buf->base != buf->head && !buf->realloc_ok) {
        free(buf->head);
    }
}

inline static size_t
oj_buf_len(ojBuf buf) {
    return buf->tail - buf->head;
}

inline static void
oj_buf_append_string(ojBuf buf, const char *s, size_t slen) {
    if (OJ_OK != buf->err) {
	return;
    }
    if (buf->end <= buf->tail + slen) {
	if (0 != buf->fd) {
	    size_t	len = buf->tail - buf->head;

	    if (len != (size_t)write(buf->fd, buf->head, len)) {
		buf->err = OJ_ERR_WRITE;
	    }
	    buf->tail = buf->head;
	} else if (buf->realloc_ok) {
	    size_t	len = buf->end - buf->head;
	    size_t	toff = buf->tail - buf->head;
	    size_t	new_len = len + slen + len;
	    //size_t	new_len = len + slen + len / 2;

	    if (buf->base == buf->head) {
		buf->head = (char*)malloc(new_len);
		memcpy(buf->head, buf->base, len);
	    } else {
		buf->head = (char*)realloc(buf->head, new_len);
	    }
	    buf->tail = buf->head + toff;
	    buf->end = buf->head + new_len - 2;
	} else {
	    slen = buf->end - buf->tail - 1;
	    buf->err = OJ_ERR_OVERFLOW;
	    return;
	}
    }
    if (0 < slen) {
	memcpy(buf->tail, s, slen);
    }
    buf->tail += slen;
    *buf->tail = '\0';
}

inline static void
oj_buf_append(ojBuf buf, char c) {
    if (OJ_OK != buf->err) {
	return;
    }
    if (buf->end <= buf->tail) {
	if (0 != buf->fd) {
	    size_t	len = buf->tail - buf->head;

	    if (len != (size_t)write(buf->fd, buf->head, len)) {
		buf->err = OJ_ERR_WRITE;
	    }
	    buf->tail = buf->head;
	} else if (buf->realloc_ok) {
	    size_t	len = buf->end - buf->head;
	    size_t	toff = buf->tail - buf->head;
	    size_t	new_len = len + len / 2;

	    if (buf->base == buf->head) {
		buf->head = (char*)malloc(new_len);
		memcpy(buf->head, buf->base, len);
	    } else {
		buf->head = (char*)realloc(buf->head, new_len);
	    }
	    buf->tail = buf->head + toff;
	    buf->end = buf->head + new_len - 2;
	} else {
	    buf->err = OJ_ERR_OVERFLOW;
	    return;
	}
    }
    *buf->tail++ = c;
    *buf->tail = '\0';
}

inline static void
oj_buf_finish(ojBuf buf) {
    if (OJ_OK != buf->err) {
	return;
    }
    if (0 != buf->fd) {
	size_t	len = buf->tail - buf->head;

	if (0 < len && len != (size_t)write(buf->fd, buf->head, len)) {
	    buf->err = OJ_ERR_WRITE;
	}
	buf->tail = buf->head;
    } else {
	*buf->tail = '\0';
    }
}

inline static char*
oj_buf_take_string(ojBuf buf) {
    char	*str = buf->head;

    if (buf->base != buf->head && !buf->realloc_ok) {
	buf->head = NULL;
    } else {
	int	len = buf->tail - buf->head;
	char	*dup = (char*)malloc(len + 1);

	str = memcpy(dup, buf->head, len);
	str[len] = '\0';
    }
    return str;
}

#endif // OJ_BUF_H
