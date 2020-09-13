// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_BUF_H
#define OJ_BUF_H

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void	oj_buf_init(ojBuf buf, int fd);
extern void	oj_buf_finit(ojBuf buf, char *str, size_t slen);
extern char*	oj_buf_take_string(ojBuf buf);
extern void	oj_buf_cleanup(ojBuf buf);
extern void	oj_buf_finish(ojBuf buf);

extern void	_oj_buf_grow(ojBuf buf, size_t slen);

inline static void
oj_buf_reset(ojBuf buf) {
    buf->tail = buf->head;
    *buf->head = '\0';
    buf->err = OJ_OK;
}

inline static size_t
oj_buf_len(ojBuf buf) {
    return buf->tail - buf->head;
}

inline static void
oj_buf_append_string(ojBuf buf, const char *s, size_t slen) {
    if (OJ_OK == buf->err) {
	if (buf->end <= buf->tail + slen) {
	    if (0 != buf->fd) {
		size_t	len = buf->tail - buf->head;

		if (len != (size_t)write(buf->fd, buf->head, len)) {
		    buf->err = OJ_ERR_WRITE;
		}
		buf->tail = buf->head;
	    } else if (buf->realloc_ok) {
		_oj_buf_grow(buf, slen);
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
}

inline static void
oj_buf_append(ojBuf buf, char c) {
    if (OJ_OK == buf->err) {
	if (buf->end <= buf->tail) {
	    if (0 != buf->fd) {
		size_t	len = buf->tail - buf->head;

		if (len != (size_t)write(buf->fd, buf->head, len)) {
		    buf->err = OJ_ERR_WRITE;
		}
		buf->tail = buf->head;
	    } else if (buf->realloc_ok) {
		_oj_buf_grow(buf, 1);
	    } else {
		buf->err = OJ_ERR_OVERFLOW;
		return;
	    }
	}
	*buf->tail++ = c;
	*buf->tail = '\0';
    }
}

#endif // OJ_BUF_H
