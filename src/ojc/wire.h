/* wire.h
 * Copyright (c) 2017, Peter Ohler
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

#ifndef __OJC_WIRE_H__
#define __OJC_WIRE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "types.h"

#define WIRE_STACK_SIZE	256
    
    typedef struct _ojcWire {
	uint8_t	*buf;
	uint8_t	*end;
	uint8_t	*cur;
	bool	own;
	bool	*top;
	bool	stack[WIRE_STACK_SIZE]; // true is a object, false an array
    } *ojcWire;
    
    typedef struct _ojcWireCallbacks {
	int	(*begin_obj)(ojcErr err, void *ctx);
	int	(*end_obj)(ojcErr err, void *ctx);
	int	(*key)(const char *key, int len);
	int	(*begin_array)(ojcErr err, void *ctx);
	int	(*end_array)(ojcErr err, void *ctx);
	int	(*add_null)(ojcErr err, void *ctx);
	int	(*add_int)(ojcErr err, int64_t num, void *ctx);
	int	(*add_float)(ojcErr err, double num, void *ctx);
	int	(*add_number)(ojcErr err, const char *num, int len, void *ctx);
	int	(*add_string)(ojcErr err, const char *str, int len, void *ctx);
	int	(*add_uuid)(ojcErr err , uint64_t hi, uint64_t lo, void *ctx);
	int	(*add_uuid_str)(ojcErr err, const char *str, void *ctx);
	int	(*add_time)(ojcErr err, int64_t hi, void *ctx);
	int	(*add_time_str)(ojcErr err, const char *str, void *ctx);
    } *ojcWireCallbacks;

    extern int		ojc_wire_size(ojcVal val);
    extern uint8_t*	ojc_wire_mem(ojcErr err, ojcVal val);
    extern size_t	ojc_wire_fill(ojcVal val, uint8_t *wire, size_t size);
    extern int		ojc_wire_file(ojcErr err, ojcVal val, FILE *file);
    extern int		ojc_wire_fd(ojcErr err, ojcVal val, int fd);

    extern int		ojc_wire_init(ojcErr err, ojcWire wire, uint8_t *buf, size_t size);
    extern void		ojc_wire_cleanup(ojcWire wire);
    extern int		ojc_wire_finish(ojcErr err, ojcWire wire);
    extern size_t	ojc_wire_length(ojcWire wire);

    extern uint8_t*	ojc_wire_take(ojcWire wire);
    extern int		ojc_wire_push_object(ojcErr err, ojcWire wire);
    extern int		ojc_wire_push_array(ojcErr err, ojcWire wire);
    extern int		ojc_wire_pop(ojcErr err, ojcWire wire);
    extern int		ojc_wire_push_key(ojcErr err, ojcWire wire, const char *key, int len);
    extern int		ojc_wire_push_null(ojcErr err, ojcWire wire);
    extern int		ojc_wire_push_bool(ojcErr err, ojcWire wire, bool value);
    extern int		ojc_wire_push_int(ojcErr err, ojcWire wire, int64_t value);
    extern int		ojc_wire_push_double(ojcErr err, ojcWire wire, double value);
    extern int		ojc_wire_push_string(ojcErr err, ojcWire wire, const char *value, int len);
    extern int		ojc_wire_push_uuid(ojcErr err, ojcWire wire, uint64_t hi, uint64_t lo);
    extern int		ojc_wire_push_uuid_string(ojcErr err, ojcWire wire, const char *value);
    extern int		ojc_wire_push_time(ojcErr err, ojcWire wire, uint64_t value);

    extern int		ojc_wire_to_json(ojcErr err, struct _Buf *buf, const uint8_t *wire);

    extern ojcVal	ojc_wire_parse_str(ojcErr err, const uint8_t *wire);
    extern ojcVal	ojc_wire_parse_file(ojcErr err, FILE *file);
    extern ojcVal	ojc_wire_parse_fd(ojcErr err, int fd);

    extern int		ojc_wire_cbparse_str(ojcErr err, const uint8_t *wire, ojcWireCallbacks callbacks);
    extern int		ojc_wire_cbparse_file(ojcErr err, FILE *file, ojcWireCallbacks callbacks);
    extern int		ojc_wire_cbparse_fd(ojcErr err, int fd, ojcWireCallbacks callbacks);

#ifdef __cplusplus
}
#endif
#endif /* __OJC_WIRE_H__ */
