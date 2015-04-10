/* reader.h
 * Copyright (c) 2014, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OJC_READER_H__
#define __OJC_READER_H__

#include <stdlib.h>

typedef struct _Reader {
    char	base[0x00001000];
    char	*head;
    char	*end;
    char	*tail;
    char	*read_end;	// one past last character read
    char	*pro;		// protection start, buffer can not slide past this point
    char	*start;		// start of current string being read
    int		line;
    int		col;
    int		free_head;
    bool	eof;
    bool	(*read_func)(ojcErr err, struct _Reader *reader); // return eof state
    union {
	int		socket;
	FILE		*file;
	const char	*str;
	struct {
	    void	*src;
	    ssize_t	(*rf)(void *src, char *buf, size_t size);
	};
    };
} *Reader;

extern void	reader_init_str(ojcErr err, Reader reader, const char *str);
extern void	reader_init_stream(ojcErr err, Reader reader, FILE *file);
extern void	reader_init_follow(ojcErr err, Reader reader, FILE *file);
extern void	reader_init_socket(ojcErr err, Reader reader, int socket);
extern void	reader_init_func(ojcErr err, Reader reader, void *src, ssize_t (*rf)(void *src, char *buf, size_t size));
extern bool	reader_read(ojcErr err, Reader reader);

static inline char
reader_get(ojcErr err, Reader reader) {
    //printf("*** drive get from '%s'  from start: %ld	buf: %p	 from read_end: %ld\n", reader->tail, reader->tail - reader->head, reader->head, reader->read_end - reader->tail);
    if (reader->read_end <= reader->tail) {
	if (reader->eof) {
	    return '\0';
	}
	reader->eof = reader_read(err, reader);
	if (reader->eof && reader->read_end <= reader->tail) {
	    return '\0';
	}	    
    }
    if ('\n' == *reader->tail) {
	reader->line++;
	reader->col = 0;
    }
    reader->col++;
    
    return *reader->tail++;
}

static inline void
reader_backup(Reader reader) {
    reader->tail--;
    reader->col--;
    if (0 >= reader->col) {
	reader->line--;
	// allow col to be negative since we never backup twice in a row
    }
}

static inline void
reader_protect(Reader reader) {
    reader->pro = reader->tail;
    reader->start = reader->tail; // can't have start before pro
}

static inline void
reader_release(Reader reader) {
    reader->pro = 0;
}

/* Starts by reading a character so it is safe to use with an empty or
 * compacted buffer.
 */
static inline char
reader_next_non_white(ojcErr err, Reader reader) {
    char	c;

    while ('\0' != (c = reader_get(err, reader))) {
	switch(c) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	    break;
	default:
	    return c;
	}
    }
    return '\0';
}

/* Starts by reading a character so it is safe to use with an empty or
 * compacted buffer.
 */
static inline char
reader_next_white(ojcErr err, Reader reader) {
    char	c;

    while ('\0' != (c = reader_get(err, reader))) {
	switch(c) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	case '\0':
	    return c;
	default:
	    break;
	}
    }
    return '\0';
}

static inline int
reader_expect(ojcErr err, Reader reader, const char *s) {
    for (; '\0' != *s; s++) {
	if (reader_get(err, reader) != *s) {
	    return -1;
	}
    }
    return 0;
}

static inline void
reader_cleanup(Reader reader) {
    if (reader->free_head && 0 != reader->head) {
	free(reader->head);
	reader->head = 0;
	reader->free_head = 0;
    }
}

static inline int
is_white(char c) {
    switch(c) {
    case ' ':
    case '\t':
    case '\f':
    case '\n':
    case '\r':
	return 1;
    default:
	break;
    }
    return 0;
}

#endif /* __OJC_READER_H__ */
