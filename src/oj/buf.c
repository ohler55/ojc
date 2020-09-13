
#include "oj.h"
#include "buf.h"
#include "debug.h"

void
oj_buf_init(ojBuf buf, int fd) {
    buf->head = buf->base;
    buf->end = buf->base + sizeof(buf->base) - 1;
    buf->tail = buf->head;
    buf->fd = fd;
    buf->realloc_ok = (0 == fd);
    *buf->head = '\0';
    buf->err = OJ_OK;
}

void
oj_buf_finit(ojBuf buf, char *str, size_t slen) {
    buf->head = str;
    buf->end = str + slen;
    buf->tail = buf->head;
    buf->fd = 0;
    buf->realloc_ok = false;
    buf->err = OJ_OK;
}

char*
oj_buf_take_string(ojBuf buf) {
    char	*str = buf->head;

    if (buf->base != buf->head && !buf->realloc_ok) {
	buf->head = NULL;
    } else {
	int	len = buf->tail - buf->head;
	char	*dup = (char*)OJ_MALLOC(len + 1);

	str = memcpy(dup, buf->head, len);
	str[len] = '\0';
    }
    return str;
}

void
oj_buf_cleanup(ojBuf buf) {
    if (buf->base != buf->head && buf->realloc_ok) {
        OJ_FREE(buf->head);
    }
}

void
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

void
_oj_buf_grow(ojBuf buf, size_t slen) {
    size_t	len = buf->end - buf->head;
    size_t	toff = buf->tail - buf->head;
    size_t	new_len = len + slen + len;
    //size_t	new_len = len + slen + len / 2;

    if (buf->base == buf->head) {
	buf->head = (char*)OJ_MALLOC(new_len);
	memcpy(buf->head, buf->base, len);
    } else {
	buf->head = (char*)OJ_REALLOC(buf->head, new_len);
    }
    buf->tail = buf->head + toff;
    buf->end = buf->head + new_len - 2;
}
