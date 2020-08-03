// Copyright (c) 2020, Peter Ohler, All rights reserved.

#ifndef OJ_INTERN_H
#define OJ_INTERN_H

#ifdef __cplusplus
extern "C" {
#endif

    extern void		oj_err_clear(ojErr err);
    extern ojStatus	oj_err_set(ojErr err, int code, const char *fmt, ...);
    extern ojStatus	oj_err_no(ojErr err, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif // OJ_INTERN_H
