// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_H
#define OJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define OJ_VERSION	"3.1.1"
#define OJ_ERR_INIT	{ .code = 0, .line = 0, .col = 0, .msg = { '\0' } }
#define OJ_ERR_START	300

#define OJ_ERR_MEM(err, type) agoo_err_memory(err, type, __FILE__, __LINE__)

    typedef enum {
	OJ_OK		= 0,
	OJ_ERR_MEMORY	= ENOMEM,
	OJ_ERR_DENIED	= EACCES,
	OJ_ERR_IMPL	= ENOSYS,
	OJ_ERR_PARSE	= OJ_ERR_START,
	OJ_ERR_READ,
	OJ_ERR_WRITE,
	OJ_ERR_OVERFLOW,
	OJ_ERR_ARG,
	OJ_ERR_NOT_FOUND,
	OJ_ERR_THREAD,
	OJ_ERR_NETWORK,
	OJ_ERR_LOCK,
	OJ_ERR_FREE,
	OJ_ERR_IN_USE,
	OJ_ERR_TOO_MANY,
	OJ_ERR_TYPE,
	OJ_ERR_EVAL,
	OJ_ERR_TLS,
	OJ_ERR_EOF,
	OJ_ERR_LAST,
    } ojStatus;

    typedef enum {
	OJ_NULL		= 'n',
	OJ_TRUE		= 't',
	OJ_FALSE	= 'f',
	OJ_INT		= 'i',
	OJ_DECIMAL	= 'd',
	OJ_STRING	= 's',
	OJ_OBJECT	= 'o',
	OJ_ARRAY	= 'a',
	OJ_FREE		= 'X',
    } ojType;

    typedef enum {
	OJ_STR_INLINE	= 0,
	OJ_STR_EXT	= 'x',
	OJ_OBJ_RAW	= 'r',
	OJ_OBJ_HASH	= 'h',
	OJ_INT_RAW	= 'i', // no . or e in string
	OJ_DEC_RAW	= 'd', // . or e in string
    } ojMod;

    typedef struct _ojBuf {
	char		*head;
	char		*end;
	char		*tail;
	int		fd;
	bool		realloc_ok;
	ojStatus	err;
	char		base[4096];
    } *ojBuf;

    typedef struct _ojErr {
	int		code;
	int		line;
	int		col;
	char		msg[256];
    } *ojErr;

    typedef struct _ojExt {
	struct _ojExt	*next;
	uint16_t	len;
	char		val[4080];
    } *ojExt;

    struct _ojStr {
	char		start[112]; 	// first char is the length, '\0' terminated
	int		len;		// length of combines string blocks
	ojExt		more;		// additional string blocks
    } *ojStr;

    typedef struct _ojNum {
	union {
	    int64_t	fixnum;
	    long double	dub;
	};
	int		len;
	char		raw[96]; 	// '\0' terminated
	ojExt		more;
    } *ojNum;

    typedef struct _ojList {
	// pointer is volatile
	struct _ojVal	*volatile head;
	struct _ojVal	*volatile tail;
    } *ojList;

    typedef struct _ojVal {
	struct _ojVal		*next;
	struct _ojStr		key;
	union {
	    struct _ojStr	str;
	    struct _ojList	list;
	    struct _ojVal	*hash[16];
	    struct _ojNum	num;
	};
	uint8_t			type;	// ojType
	uint8_t			mod;	// ojMod
    } *ojVal;

    typedef bool		(*ojParseCallback)(ojVal val, void *ctx);
    typedef ssize_t		(*ojReadFunc)(void *src, char *buf, size_t size);

    typedef struct _ojParser {
	const char	*map;
	const char	*next_map;
	struct _ojVal	val; // working val
	struct _ojErr	err;
	void		(*push)(ojVal val, void *ctx);
	void		(*pop)(void *ctx);
	void		*ctx; // push and pop context

	int		depth;
	int		ri;
	// TBD change this stack to be expandable
	char		stack[1024];
    } *ojParser;

    // General functions.
    extern const char*	oj_version(void);
    extern void		oj_cleanup(void);
    extern const char*	oj_type_str(ojType type);

    extern void		oj_err_init(ojErr err);
    extern const char*	oj_status_str(ojStatus code);

    extern ojStatus	oj_validate_str(ojErr err, const char *json);

    extern void		oj_parser_reset(ojParser p);

    extern ojStatus	oj_parse_str(ojParser p, const char *json);
    extern ojStatus	oj_parse_strp(ojParser p, const char **json);
    extern ojStatus	oj_parse_file(ojParser p, const char *filename);
    extern ojStatus	oj_parse_fd(ojParser p, int fd);

    extern ojStatus	oj_parse_reader(ojParser p, void *src, ojReadFunc rf);
    extern ojStatus	oj_parse_file_follow(ojParser p, FILE *file);

    extern void		oj_val_parser(ojParser p);
    extern ojVal	oj_val_parse_str(ojErr err, const char *json, ojParseCallback cb, void *ctx);
    extern ojVal	oj_val_parse_file(ojErr err, const char *filename, ojParseCallback cb, void *ctx);
    extern ojVal	oj_val_parse_fd(ojErr err, int fd, ojParseCallback cb, void *ctx);

    extern ojVal	oj_val_parse_strp(ojErr err, const char **json);
    extern ojVal	oj_val_parse_reader(ojErr err, void *src, ojReadFunc rf);

    // TBD val access functions
    // TBD val creation functions
    extern ojVal	oj_val_create();
    extern void		oj_destroy(ojVal val);

    extern char*	oj_to_str(ojVal val, int indent);
    extern size_t	oj_fill(ojErr err, ojVal val, int indent, char *buf, int max);
    extern size_t	oj_buf(ojBuf buf, ojVal val, int indent, int depth);
    extern size_t	oj_write(ojErr err, ojVal val, int indent, int socket);
    extern size_t	oj_fwrite(ojErr err, ojVal val, int indent, FILE *file);

    extern bool		oj_thread_safe;

#ifdef __cplusplus
}
#endif
#endif // OJ_H
