// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_H
#define OJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define OJ_VERSION_MAJOR	4
#define OJ_VERSION_MINOR	0
#define OJ_VERSION_PATCH	0
#define OJ_VERSION		"4.0.0"

#define OJ_ERR_INIT		{ .code = 0, .line = 0, .col = 0, .msg = { '\0' } }
#define OJ_ERR_START		300
#define OJ_BUILDER_INIT		{ .top = NULL, .stack = NULL, .err = { .code = 0, .line = 0, .col = 0, .msg = { '\0' } } }

    typedef enum {
	OJ_OK		= 0,
	OJ_ERR_MEMORY	= ENOMEM,
	OJ_ERR_PARSE	= OJ_ERR_START,
	OJ_ERR_READ,
	OJ_ERR_WRITE,
	OJ_ERR_OVERFLOW,
	OJ_ERR_ARG,
	OJ_ERR_TOO_MANY,
	OJ_ERR_TYPE,
	OJ_ERR_KEY,
	OJ_ABORT,
	OJ_ERR_LAST,
    } ojStatus;

    typedef enum {
	OJ_NONE		= '\0',
	OJ_NULL		= 'n',
	OJ_TRUE		= 't',
	OJ_FALSE	= 'f',
	OJ_INT		= 'i',
	OJ_DECIMAL	= 'd',
	OJ_BIG		= 'b',
	OJ_STRING	= 's',
	OJ_OBJECT	= 'o',
	OJ_ARRAY	= 'a',
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

    typedef union _ojS4k {
	union _ojS4k	*next;
	char		str[4096];
    } *ojS4k;

    typedef struct _ojStr {
	int		len;	// length of raw or ptr excluding \0
	union {
	    char	raw[120];
	    ojS4k	s4k;
	    struct {
		size_t	cap;
		char	*ptr;
	    };
	};
    } *ojStr;

    typedef struct _ojNum {
	long double	dub;
	int64_t		fixnum; // holds all digits
	uint32_t	len;
	int16_t		div;	// 10^div
	int16_t		exp;
	uint8_t		shift;	// shift of fixnum to get decimal
	bool		neg;
	bool		exp_neg;
	bool		calc;	// if true value has been calculated
	union {
	    char	raw[88];
	    ojS4k	s4k;
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
    typedef void		(*ojPushFunc)(ojVal val, void *ctx);
    typedef void		(*ojPopFunc)(void *ctx);

    typedef struct _ojReuser {
	ojVal			head;
	ojVal			tail;
	ojVal			dig;
    } *ojReuser;

    typedef struct _ojCall {
	ojVal			val;
	struct _ojReuser	reuser;
	atomic_flag		busy;
    } *ojCall;

    typedef struct _ojCaller {
	struct _ojCall		queue[256];
	pthread_t		thread;
	ojParseCallback		cb;
	void			*ctx;
	ojCall			end;
	ojCall			tail;
	atomic_flag		starting;
	volatile bool		done;
    } *ojCaller;

    typedef struct _ojBuilder {
	ojVal			top;
	ojVal			stack;
	struct _ojErr		err;
    } *ojBuilder;

    // General functions.
    extern const char*	oj_version(void);
    extern void		oj_cleanup(void);
    extern const char*	oj_type_str(ojType type);

    extern void		oj_err_init(ojErr err);
    extern const char*	oj_status_str(ojStatus code);

    extern ojStatus	oj_caller_start(ojErr err, ojCaller caller, ojParseCallback cb, void *ctx);
    extern void		oj_caller_shutdown(ojCaller caller);
    extern void		oj_caller_wait(ojCaller caller);

    extern ojStatus	oj_validate_str(ojErr err, const char *json);

    extern ojVal	oj_parse_str(ojErr err, const char *json, ojReuser reuser);
    extern ojVal	oj_parse_strp(ojErr err, const char **json, ojReuser reuser);
    extern ojStatus	oj_parse_str_cb(ojErr err, const char *json, ojParseCallback cb, void *ctx);
    extern ojStatus	oj_parse_str_call(ojErr err, const char *json, ojCaller caller);
    extern ojStatus	oj_pp_parse_str(ojErr		err,
					const char	*json,
					ojPushFunc	push,
					ojPopFunc	pop,
					void		*ctx);

    extern ojVal	oj_parse_fd(ojErr err, int fd, ojReuser reuser);
    extern ojStatus	oj_parse_fd_cb(ojErr err, int fd, ojParseCallback cb, void *ctx);
    extern ojStatus	oj_parse_fd_call(ojErr err, int fd, ojCaller caller);
    extern ojStatus	oj_pp_parse_fd(ojErr		err,
				       int		fd,
				       ojPushFunc	push,
				       ojPopFunc	pop,
				       void		*ctx);

    extern ojVal	oj_parse_file(ojErr err, const char *filename, ojReuser reuser);
    extern ojStatus	oj_parse_file_cb(ojErr err, const char *filename, ojParseCallback cb, void *ctx);
    extern ojStatus	oj_parse_file_call(ojErr err, const char *filename, ojCaller caller);
    extern ojStatus	oj_pp_parse_file(ojErr		err,
					 const char	*filepath,
					 ojPushFunc	push,
					 ojPopFunc	pop,
					 void		*ctx);

    extern ojVal	oj_val_create();
    extern void		oj_destroy(ojVal val);
    extern void		oj_reuse(ojReuser reuser);

    extern void		oj_null_set(ojVal val);
    extern void		oj_bool_set(ojVal val, bool b);
    extern void		oj_int_set(ojVal val, int64_t fixnum);
    extern void		oj_double_set(ojVal val, long double dub);
    extern ojStatus	oj_str_set(ojErr err, ojVal val, const char *s, size_t len);
    extern ojStatus	oj_key_set(ojErr err, ojVal val, const char *key, size_t len);
    extern ojStatus	oj_bignum_set(ojErr err, ojVal val, const char *s, size_t len);
    extern ojStatus	oj_append(ojErr err, ojVal val, ojVal member);
    extern ojStatus	oj_object_set(ojErr err, ojVal val, const char *key, ojVal member);

    extern ojVal	oj_null_create(ojErr err);
    extern ojVal	oj_bool_create(ojErr err, bool b);
    extern ojVal	oj_str_create(ojErr err, const char *s, size_t len);
    extern ojVal	oj_int_create(ojErr err, int64_t fixnum);
    extern ojVal	oj_double_create(ojErr err, long double dub);
    extern ojVal	oj_bignum_create(ojErr err, const char *s, size_t len);
    extern ojVal	oj_array_create(ojErr err);
    extern ojVal	oj_object_create(ojErr err);

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
    extern size_t	oj_write(ojErr err, ojVal val, int indent, int fd);
    extern size_t	oj_fwrite(ojErr err, ojVal val, int indent, const char *filepath);

    extern ojStatus	oj_build_object(ojBuilder b, const char *key);
    extern ojStatus	oj_build_array(ojBuilder b, const char *key);
    extern ojStatus	oj_build_null(ojBuilder b, const char *key);
    extern ojStatus	oj_build_bool(ojBuilder b, const char *key, bool boo);
    extern ojStatus	oj_build_int(ojBuilder b, const char *key, int64_t i);
    extern ojStatus	oj_build_double(ojBuilder b, const char *key, long double d);
    extern ojStatus	oj_build_string(ojBuilder b, const char *key, const char *s, size_t len);
    extern ojStatus	oj_build_bignum(ojBuilder b, const char *key, const char *big, size_t len);
    extern ojStatus	oj_build_pop(ojBuilder b);
    extern void		oj_build_popall(ojBuilder b);

    extern bool		oj_thread_safe;

#ifdef __cplusplus
}
#endif
#endif // OJ_H
