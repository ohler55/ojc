// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_INTERN_H
#define OJ_INTERN_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef uint8_t	byte;

    extern void		oj_err_clear(ojErr err);
    extern ojStatus	oj_err_set(ojErr err, int code, const char *fmt, ...);
    extern ojStatus	oj_err_no(ojErr err, const char *fmt, ...);

    extern void		_oj_val_set_str(ojParser p, const char *s, size_t len);
    extern void		_oj_val_append_str(ojParser p, const byte *s, size_t len);
    extern void		_oj_val_set_key(ojParser p, const char *s, size_t len);

#ifdef __cplusplus
}
#endif
#endif // OJ_INTERN_H
