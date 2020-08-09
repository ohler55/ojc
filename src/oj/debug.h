// Copyright (c) 2018, Peter Ohler, All rights reserved.

#ifndef OJ_DEBUG_H
#define OJ_DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MEM_DEBUG

#define OJ_MALLOC(size) oj_malloc(size, __FILE__, __LINE__)
#define OJ_CALLOC(count, size) oj_calloc(count, size, __FILE__, __LINE__)
#define OJ_REALLOC(ptr, size) oj_realloc(ptr, size, __FILE__, __LINE__)

#define OJ_FREE(ptr) oj_free(ptr, __FILE__, __LINE__)
#define OJ_MEM_CHECK(ptr) oj_mem_check(ptr, __FILE__, __LINE__)

extern void*	oj_malloc(size_t size, const char *file, int line);
extern void*	oj_calloc(size_t count, size_t size, const char *file, int line);
extern void*	oj_realloc(void *ptr, size_t size, const char *file, int line);
extern void	oj_free(void *ptr, const char *file, int line);
extern void	oj_mem_check(void *ptr, const char *file, int line);

#else

#define OJ_MALLOC(size) malloc(size)
#define OJ_CALLOC(count, size) calloc(count, size)
#define OJ_REALLOC(ptr, size) realloc(ptr, size)
#define OJ_FREE(ptr) free(ptr)
#define OJ_MEM_CHECK(ptr) {}

#endif

extern void	debug_report();

#endif /* OJ_DEBUG_H */
