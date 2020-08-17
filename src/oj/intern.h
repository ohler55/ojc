// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_INTERN_H
#define OJ_INTERN_H

#ifdef __cplusplus
extern "C" {
#endif

#define OJ_ERR_MEM(err, type) oj_err_memory(err, type, __FILE__, __LINE__)

    typedef uint8_t	byte;

    extern void		oj_err_clear(ojErr err);
    extern int		oj_err_memory(ojErr err, const char *type, const char *file, int line);
    extern ojStatus	oj_err_set(ojErr err, int code, const char *fmt, ...);
    extern ojStatus	oj_err_no(ojErr err, const char *fmt, ...);

    extern void		_oj_val_set_str(ojVal val, const char *s, size_t len);
    extern void		_oj_val_set_key(ojVal val, const char *s, size_t len);
    extern void		_oj_append_str(ojParser p, ojStr str, const byte *s, size_t len);
    extern void		_oj_append_num(ojParser p, const char *s, size_t len);
    extern void		_oj_fast_destroy(ojVal head, ojVal tail, ojVal dig);
    extern void		_oj_val_clear(ojVal v);

#ifdef __cplusplus
}
#endif
#endif // OJ_INTERN_H
