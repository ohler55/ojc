// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_H
#define OJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
//#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define OJ_VERSION	"3.1.1"
#define OJ_ERR_INIT	{ .code = 0, .msg = { 0 } }

    typedef enum {
	OJ_OK			= 0,
	OJ_TYPE_ERR		= 't',
	OJ_PARSE_ERR		= 'p',
	OJ_INCOMPLETE_ERR	= 'i',
	OJ_OVERFLOW_ERR		= 'o',
	OJ_WRITE_ERR		= 'w',
	OJ_MEMORY_ERR		= 'm',
	OJ_UNICODE_ERR		= 'u',
	OJ_ABORT_ERR		= '@',
	OJ_ARG_ERR		= 'a',
    } ojStatus;

    typedef enum {
	OJ_NIL		= 'n',
	OJ_TRUE		= 't',
	OJ_FALSE	= 'f',
	OJ_INT		= 'i',
	OJ_FLOAT	= 'd',
	OJ_STRING	= 's',
	OJ_OBJECT	= 'o',
	OJ_ARRAY	= 'a',
    } ojType;

    typedef enum {
	OJ_STR_INLINE	= 0,
	OJ_STR_EXT	= 'x',
	OJ_OBJ_RAW	= 'r',
	OJ_OBJ_HASH	= 'h',
	OJ_INT_RAW	= 'i', // no . or e in string
	OJ_DEC_RAW	= 'd', // . or e in string
    } ojMod;

#include "buf.h"

    typedef struct _ojErr {
	int	code;
	char	msg[256];
    } *ojErr;

    typedef struct _ojExt {
	struct _ojExt	*next;
	uint16_t	len;
	char		val[4080];
    } *ojExt;

    union _ojStr {
	char		val[64]; 	// first char is the length
	struct {
	    char	start[48]; 	// start of string, first char is 0x7f
	    int		len;		// length of combines string blocks
	    ojExt	more;		// additional string blocks
	};
    } *ojStr;

    typedef struct _ojVal {
	struct _ojVal		*next;
	union _ojStr		key;
	union {
	    union _ojStr	str;
	    struct _ojVal	*list;
	    struct _ojVal	*hash[8];
	    int64_t		fixnum;
	    long double		dub;
	};
	uint32_t		str_len;
	uint16_t		key_len;
	uint8_t			type;	// ojType
	uint8_t			mod;	// ojMod
    } *ojVal;

    typedef bool		(*ojParseCallback)(ojErr err, ojVal val, void *ctx);
    typedef ssize_t		(*ojReadFunc)(void *src, char *buf, size_t size);

    typedef struct _ojParser {
	void		(*push)(ojVal val);
	void		(*pop)();
	ojParseCallback	cb;
	void		*ctx;
	struct _ojErr	err;
    } *ojParser;

    // General functions.
    extern const char*	oj_version(void);
    extern void		oj_cleanup(void);
    extern const char*	oj_type_str(ojType type);
    extern const char*	oj_status_str(ojStatus code);

    extern void		oj_validator(ojParser p);

    extern ojStatus	oj_parse_str(ojParser p, const char *json);
    extern ojStatus	oj_parse_strp(ojParser p, const char **json);
    extern ojStatus	oj_parse_file(ojParser p, FILE *file);
    extern ojStatus	oj_parse_fd(ojParser p, int socket);
    extern ojStatus	oj_parse_reader(ojParser p, void *src, ojReadFunc rf);
    extern ojStatus	oj_parse_file_follow(ojParser p, FILE *file);

    extern void		oj_val_parser(ojParser p);
    extern ojVal	oj_val_parse_str(ojErr err, const char *json);
    extern ojVal	oj_val_parse_strp(ojErr err, const char **json);
    extern ojVal	oj_val_parse_file(ojErr err, FILE *file);
    extern ojVal	oj_val_parse_fd(ojErr err, int socket);
    extern ojVal	oj_val_parse_reader(ojErr err, void *src, ojReadFunc rf);

    // TBD val access functions
    // TBD val creation functions

    extern char*	oj_to_str(ojVal val, int indent);
    extern size_t	oj_fill(ojErr err, ojVal val, int indent, char *buf, int max);
    extern size_t	oj_buf(ojBuf buf, ojVal val, int indent, int depth);
    extern size_t	oj_write(ojErr err, ojVal val, int indent, int socket);
    extern size_t	oj_fwrite(ojErr err, ojVal val, int indent, FILE *file);

    static inline void
    oj_err_init(ojErr err) {
	err->code = OJ_OK;
	*err->msg = '\0';
    }

#ifdef __cplusplus
}
#endif
#endif // OJ_H
