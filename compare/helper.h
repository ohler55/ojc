// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#ifndef OJ_HELPER_H
#define OJ_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

    extern uint64_t	clock_micro();
    extern char*	mem_use(char *buf, size_t size);
    extern char*	load_file(const char *filename);
    extern void		form_json_results(const char *name, long long iter, long long usec, const char *mem, const char *err);


#ifdef __cplusplus
}
#endif
#endif // OJ_HELPER_H
