/* reader.c
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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#if NEEDS_UIO
#include <sys/uio.h>	
#endif
#include <unistd.h>
#include <time.h>

#include "ojc.h"
#include "reader.h"

#define BUF_PAD	4

static bool	read_from_file(ojcErr err, Reader reader);
static bool	read_from_socket(ojcErr err, Reader reader);
static bool	read_from_str(ojcErr err, Reader reader);

static void
reader_init(Reader reader) {
    reader->head = reader->base;
    *((char*)reader->head) = '\0';
    reader->end = reader->head + sizeof(reader->base) - BUF_PAD;
    reader->tail = reader->head;
    reader->read_end = reader->head;
    reader->pro = 0;
    reader->start = 0;
    reader->line = 1;
    reader->col = 0;
    reader->free_head = 0;
    reader->eof = false;
    reader->read_func = 0;
}

void
reader_init_str(ojcErr err, Reader reader, const char *str) {
    if (0 == str) {
	snprintf(err->msg, sizeof(err->msg) - 1, "No source string provided during initialization.");
	return;
    }
    reader_init(reader);
    reader->read_func = read_from_str;
    reader->str = str;
    reader->head = (char*)reader->str;
    reader->tail = reader->head;
    reader->read_end = reader->head + strlen(str);
    reader->eof = true;
}

void
reader_init_stream(ojcErr err, Reader reader, FILE *file) {
    if (0 == file) {
	snprintf(err->msg, sizeof(err->msg) - 1, "No source file provided during initialization.");
	return;
    }
    reader_init(reader);
    reader->read_func = read_from_file;
    reader->file = file;
}

void
reader_init_socket(ojcErr err, Reader reader, int socket) {
    if (0 >= socket) {
	snprintf(err->msg, sizeof(err->msg) - 1, "Invalid socket provided during initialization.");
	return;
    }
    reader_init(reader);
    reader->read_func = read_from_socket;
    reader->socket = socket;
}

// returns true if EOF
bool
reader_read(ojcErr err, Reader reader) {
    size_t	shift = 0;
    
    if (0 == reader->read_func) {
	snprintf(err->msg, sizeof(err->msg) - 1, "Not initialized.");
	return true;
    }
    if (reader->eof) {
	return true;
    }
    // if there is not much room to read into, shift or realloc a larger buffer.
    if (reader->head < reader->tail && 4096 > reader->end - reader->tail) {
	if (0 == reader->pro) {
	    shift = reader->tail - reader->head;
	} else {
	    shift = reader->pro - reader->head - 1; // leave one character so we can backup one
	}
	if (0 >= shift) { /* no space left so allocate more */
	    char	*old = reader->head;
	    size_t	size = reader->end - reader->head + BUF_PAD;
	
	    if (reader->head == reader->base) {
		reader->head = (char*)malloc(size * 2);
		memcpy(reader->head, old, size);
	    } else {
		reader->head = (char*)realloc(reader->head, size * 2);
	    }
	    reader->free_head = 1;
	    reader->end = reader->head + size * 2 - BUF_PAD;
	    reader->tail = reader->head + (reader->tail - old);
	    reader->read_end = reader->head + (reader->read_end - old);
	    if (0 != reader->pro) {
		reader->pro = reader->head + (reader->pro - old);
	    }
	    if (0 != reader->start) {
		reader->start = reader->head + (reader->start - old);
	    }
	} else {
	    memmove(reader->head, reader->head + shift, reader->read_end - (reader->head + shift));
	    reader->tail -= shift;
	    reader->read_end -= shift;
	    if (0 != reader->pro) {
		reader->pro -= shift;
	    }
	    if (0 != reader->start) {
		reader->start -= shift;
	    }
	}
    }
    reader->eof = reader->read_func(err, reader);
    *reader->read_end = '\0';

    return reader->eof;
}

static bool
read_from_file(ojcErr err, Reader reader) {
    ssize_t	cnt;
    size_t	max = reader->end - reader->tail;
    bool	eof = false;

    cnt = fread(reader->tail, 1, max, reader->file);
    if (cnt != max) {
	eof = true;
	if (!feof(reader->file)) {
	    snprintf(err->msg, sizeof(err->msg) - 1, "Error while reading from file.");
	    return eof;
	}
    }
    reader->read_end = reader->tail + cnt;

    return eof;
}

static bool
read_from_socket(ojcErr err, Reader reader) {
    ssize_t	cnt;
    size_t	max = reader->end - reader->tail;

    cnt = read(reader->socket, reader->tail, max);
    if (cnt < 0) {
	snprintf(err->msg, sizeof(err->msg) - 1, "Error while reading from socket %d. %s.", reader->socket, strerror(errno));
	return true;
    }
    reader->read_end = reader->tail + cnt;

    return (0 == cnt);
}

// This is only called when the end of the string is reached so just return eof (true).
static bool
read_from_str(ojcErr err, Reader reader) {
    return true;
}
