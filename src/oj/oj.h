// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_H
#define OJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define OJ_VERSION	"3.1.1"
#define OJ_ERR_INIT	{ .code = 0, .line = 0, .col = 0, .msg = { '\0' } }
#define OJ_ERR_START	300

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
	OJ_ABORT,
	OJ_ERR_LAST,
    } ojStatus;

    typedef enum {
	OJ_NULL		= 'n',
	OJ_TRUE		= 't',
	OJ_FALSE	= 'f',
	OJ_INT		= 'i',
	OJ_DECIMAL	= 'd',
	OJ_BIG		= 'b',
	OJ_STRING	= 's',
	OJ_OBJECT	= 'o',
	OJ_ARRAY	= 'a',
	OJ_FREE		= 'X',
    } ojType;

    typedef enum {
	OJ_CONTINUE	= 0x00,
	OJ_STOP		= 0x01,
	OJ_DESTROY	= 0x02,
    } ojCallbackOp;

    typedef enum {
	OJ_OBJ_RAW	= '\0',
	OJ_OBJ_HASH	= 'h',
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

    union ojS4k {
	union ojS4k	*next;
	char		str[4096];
    };

    typedef struct _ojStr {
	int		len;	// length of raw or ptr excluding \0
	union {
	    char	raw[120];
	    union ojS4k	*s4k;
	    struct {
		size_t	cap;
		char	*ptr;
	    };
	};
    } *ojStr;

    typedef struct _ojNum {
	long double	dub;
	int64_t		fixnum; // holds all digits
	uint32_t	len;	// TBD leave at 0 until building string
	int16_t		div;	// 10^div
	int16_t		exp;
	uint8_t		shift;	// shift of fixnum to get decimal
	bool		neg;
	bool		exp_neg;
	bool		calc;	// if true value has been calculated
	union {
	    char	raw[88];
	    union ojS4k	*s4k;
	    struct {
		size_t	cap;
		char	*ptr;
	    };
	};
    } *ojNum;

    typedef struct _ojList {
	struct _ojVal	*head;
	struct _ojVal	*tail;
    } *ojList;

    typedef struct _ojVal {
	struct _ojVal		*next;
	struct _ojVal		*free;
	struct _ojStr		key;
	uint32_t		kh;	// key hash
	uint8_t			type;	// ojType
	uint8_t			mod;	// ojMod
	union {
	    struct _ojStr	str;
	    struct _ojList	list;
	    struct _ojVal	*hash[16];
	    struct _ojNum	num;
	};
    } *ojVal;

    typedef ojCallbackOp	(*ojParseCallback)(ojVal val, void *ctx);
    typedef ssize_t		(*ojReadFunc)(void *src, char *buf, size_t size);

    typedef struct _ojReuser {
	ojVal		head;
	ojVal		tail;
	ojVal		dig;
    } *ojReuser;

    typedef struct _ojValMu {
	ojVal			val;
	struct _ojReuser	reuser;
	pthread_mutex_t		mu;
    } *ojValMu;

    typedef struct _ojCaller {
	pthread_t		thread;
	struct _ojValMu		queue[16];
	ojValMu			end;
	ojParseCallback		cb;
	void			*ctx;
	ojValMu			qp;
    } *ojCaller;

    typedef struct _ojParser {
	const char	*map;
	const char	*next_map;
	ojVal		stack;
	ojVal		results;
	struct _ojErr	err;
	ojVal		all_head;
	ojVal		all_tail;
	ojVal		all_dig;

	// for push-pull parser
	void		(*push)(ojVal val, void *ctx);
	void		(*pop)(void *ctx);
	ojVal		ready;

	ojParseCallback	cb;
	void		*ctx;

	ojCaller	caller;

	char		token[8];
	int		ri;
	uint32_t	ucode;
	// TBD maybe an enum instead of 3 bools (check performance?)
	bool		pp;
	bool		has_cb;
	bool		has_caller;
    } *ojParser;

    // General functions.
    extern const char*	oj_version(void);
    extern void		oj_cleanup(void);
    extern const char*	oj_type_str(ojType type);

    extern void		oj_err_init(ojErr err);
    extern const char*	oj_status_str(ojStatus code);

    extern ojStatus	oj_caller_start(ojErr err, ojCaller caller, ojParseCallback cb, void *ctx);
    extern void		oj_caller_push(ojParser p, ojCaller caller, ojVal val);
    extern void		oj_caller_shutdown(ojCaller caller);
    extern void		oj_caller_wait(ojCaller caller);

    extern ojStatus	oj_validate_str(ojErr err, const char *json);

    extern ojVal	oj_parse_str(ojErr err, const char *json, ojParseCallback cb, void *ctx);
    extern ojVal	oj_parse_str_reuse(ojErr err, const char *json, ojReuser reuser);
    extern ojStatus	oj_parse_str_call(ojErr err, const char *json, ojCaller caller);

    extern ojVal	oj_parse_file(ojErr err, const char *filename, ojParseCallback cb, void *ctx);
    extern ojVal	oj_parse_file_reuse(ojErr err, const char *filename, ojReuser reuser);
    extern ojStatus	oj_parse_file_call(ojErr err, const char *filename, ojCaller caller);

    extern ojVal	oj_parse_fd(ojErr err, int fd, ojParseCallback cb, void *ctx);
    extern ojVal	oj_parse_fd_reuse(ojErr err, int fd, ojReuser reuser);
    extern ojStatus	oj_parse_fd_call(ojErr err, int fd, ojCaller caller);

    extern ojStatus	oj_pp_parse_str(ojErr		err,
					const char	*json,
					void		(*push)(ojVal val, void *ctx),
					void		(*pop)(void * ctx),
					void		*ctx);


    extern ojStatus	oj_parse_strp(ojParser p, const char **json);
    extern ojStatus	oj_parse_reader(ojParser p, void *src, ojReadFunc rf);
    extern ojStatus	oj_parse_file_follow(ojParser p, FILE *file);

    extern ojVal	oj_val_create();
    extern void		oj_destroy(ojVal val);
    extern void		oj_reuse(ojReuser reuser);

    extern ojStatus	oj_val_set_str(ojErr err, ojVal val, const char *s, size_t len);
    // TBD set functions and add/append/insert
    // TBD create functions

    extern const char*	oj_key(ojVal val);
    extern const char*	oj_str_get(ojVal val);
    extern int64_t	oj_int_get(ojVal val);
    extern long double	oj_double_get(ojVal val, bool prec);
    extern const char*	oj_bignum_get(ojVal val);
    extern ojVal	oj_array_first(ojVal val);
    extern ojVal	oj_array_last(ojVal val);
    extern ojVal	oj_array_nth(ojVal val, int n);
    extern ojVal	oj_object_get(ojVal val, const char *key, int len);
    extern ojVal	oj_object_find(ojVal val, const char *key, int len);
    // for object and list, if cb return false then stop
    extern ojVal	oj_each(ojVal val, bool (*cb)(ojVal v, void* ctx), void *ctx);

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
