/* ojc.h
 * Copyright (c) 2014, Peter Ohler
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

#ifndef __OJC_H__
#define __OJC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Current version of OjC.
 */
#define OJC_VERSION	"1.4.3"

#define OJC_ERR_INIT	{ 0, { 0 } }

/**
 * The value types found in JSON documents and in the __ojcVal__ type.
 * @see ojcVal
 */
typedef enum {
    /** null */
    OJC_NULL	= 0,
    /** string */
    OJC_STRING	= 's',
    /** number as a string */
    OJC_NUMBER	= 'N',
    /** integer */
    OJC_FIXNUM	= 'n',
    /** decimal number */
    OJC_DECIMAL	= 'd',
    /** true */
    OJC_TRUE	= 't',
    /** false */
    OJC_FALSE	= 'f',
    /** array */
    OJC_ARRAY	= 'a',
    /** object */
    OJC_OBJECT	= 'o',
    /** word with a maximum of 15 characters */
    OJC_WORD	= 'w',
} ojcValType;

/**
 * Error codes for the __code__ field in __ojcErr__ structs.
 * @see ojcErr
 */
typedef enum {
    /** okay, no error */
    OJC_OK		= 0,
    /** type error */
    OJC_TYPE_ERR	= 't',
    /** parse error */
    OJC_PARSE_ERR	= 'p',
    /** buffer overflow error */
    OJC_OVERFLOW_ERR	= 'o',
    /** write error */
    OJC_WRITE_ERR	= 'w',
    /** memory error */
    OJC_MEMORY_ERR	= 'm',
    /** unicode error */
    OJC_UNICODE_ERR	= 'u',
    /** abort callback parsing */
    OJC_ABORT_ERR	= 'A',
    /** argument error */
    OJC_ARG_ERR		= 'a',
} ojcErrCode;

/**
 * The struct used to report errors or status after a function returns. The
 * struct must be initialized before use as most calls that take an err argument
 * will return immediately if an error has already occurred.
 *
 * @see ojcErrCode
 */
typedef struct _ojcErr {
    /** Error code identifying the type of error. */
    int		code;
    /** Error message associated with a failure if the code is not __OJC_OK__. */
    char	msg[256];
} *ojcErr;

/**
 * The type representing a value in a JSON file. Values will be one of the
 * __ojcValType__ enum.
 * @see ojcValType
 */
typedef struct _ojcVal	*ojcVal;

/**
 * When using the parser callbacks, the callback must be of this type. The
 * arguments will be the context provided when calling the parser and the value
 * extracted from the JSON stream.
 *
 * A return value of __true__ indicates the parser should free the values after
 * the callback returns. If the return value is false then the callback is
 * responsible for freeing the value when no longer needed.
 *
 * @see ojc_parse_str, ojc_parse_stream, ojc_parse_socket
 */
typedef bool	(*ojcParseCallback)(ojcErr err, ojcVal val, void *ctx);

/**
 * If true new lines will not be escaped in the output. This is not a JSON
 * feature.
 */
extern bool		ojc_newline_ok;

/**
 * If true non-quoted word will be returned as OJC_WORD types. This is not a
 * JSON feature.
 */
extern bool		ojc_word_ok;

/**
 * @return version of OjC.
 */
extern const char*	ojc_version(void);

/**
 * Cleans up re-use memory pools.
 */
extern void		ojc_cleanup(void);

/**
 * Parses a string. An error will result in the __err__ argument being set with
 * an error code and message. The __json__ string can contain more than one JSON
 * element.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param json JSON document to parse
 * @param cb callback function or NULL if no callbacks are desired
 * @param ctx context for the callback function if provided.
 * @return the parsed JSON as a __ojcVal__.
 * @see ojcParseCallback
 */
extern ojcVal		ojc_parse_str(ojcErr err, const char *json, ojcParseCallback cb, void *ctx);

/**
 * Parses a the contents of a file. An error will result in the __err__ argument
 * being set with an error code and message. The __file__ can contain more than
 * one JSON element.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param  file file to parse
 * @param cb callback function or NULL if no callbacks are desired
 * @param ctx context for the callback function if provided.
 * @return the parsed JSON as a __ojcVal__.
 * @see ojcParseCallback
 */
extern ojcVal		ojc_parse_stream(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx);

/**
 * Parses a stream from a socket. An error will result in the __err__ argument
 * being set with an error code and message. The __socket__ can contain more
 * than one JSON element.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param socket socket to parse
 * @param cb callback function or NULL if no callbacks are desired
 * @param ctx context for the callback function if provided.
 * @return the parsed JSON as a __ojcVal__.
 * @see ojcParseCallback
 */
extern ojcVal		ojc_parse_socket(ojcErr err, int socket, ojcParseCallback cb, void *ctx);

/**
 * Frees memory used by a __ojcVal__.
 *
 * @param val value to free or destroy.
 */
extern void		ojc_destroy(ojcVal val);

/**
 * Gets a __ojcVal__ located by the provided path where the path is a sequence
 * of keys or indices separated by the / character. If the element does not
 * exist a NULL value is returned. A NULL path returns the __val__ argument.
 *
 * @param val anchor value for locating the target value
 * @param path location of the target value
 * @return the value identified by the path or NULL if it does not exist.
 */
extern ojcVal		ojc_get(ojcVal val, const char *path);

/**
 * Gets a __ojcVal__ located by the provided path where the path is a NULL
 * terminated array of strings that are keys or indices that form a path to a
 * target value. If the element does not exist a NULL value is returned. A NULL
 * path returns the __val__ argument.
 *
 * @param val anchor value for locating the target value
 * @param path location of the target value
 * @return the value identified by the path or NULL if it does not exist.
 */
extern ojcVal		ojc_aget(ojcVal val, const char **path);

/**
 * Returns the type of the value.
 *
 * @param val value to get the type of
 * @return the type of the value.
 * @see ojcValType
 */
extern ojcValType	ojc_type(ojcVal val);

/**
 * Get the integer value of a __ojcVal__ if it is of type __OJC_FIXNUM__. If it
 * is not the correct type a type error is returned in the __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the C int64_t from
 * @return the C integer value of the __val__ argument.
 */
extern int64_t		ojc_int(ojcErr err, ojcVal val);

/**
 * Get the decimal (double) value of a __ojcVal__ if it is of type
 * __OJC_DECIMAL__. If it is not the correct type a type error is returned in
 * the __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the C double from
 * @return the C double value of the __val__ argument.
 */
extern double		ojc_double(ojcErr err, ojcVal val);

/**
 * Get the string value of a __ojcVal__ if it is of type __OJC_STRING__. If it
 * is not the correct type a type error is returned in the __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the C const char* from
 * @return the C string value of the __val__ argument.
 */
extern const char*	ojc_str(ojcErr err, ojcVal val);

/**
 * Get the string value of a __ojcVal__ if it is of type __OJC_WORD__. If it is
 * not the correct type a type error is returned in the __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the C const char* from
 * @return the C string value of the __val__ argument.
 */
extern const char*	ojc_word(ojcErr err, ojcVal val);

/**
 * Get the first child value of a __ojcVal__ if it is of type __OJC_OBJECT__ or
 * __OJC_ARRAY__. If it is not the correct type a type error is returned in the
 * __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the members of
 * @return the first member value of the __val__ argument.
 */
extern ojcVal		ojc_members(ojcErr err, ojcVal val);

/**
 * Get the next member value of a __ojcVal__. NULL is returned if there are not
 * more members.
 *
 * @param val __ojcVal__ to get the next member from
 * @return the next member value of the __val__ argument.
 */
extern ojcVal		ojc_next(ojcVal val);

/**
 * Get number of members of a __ojcVal__ if it is of type __OJC_OBJECT__ or
 * __OJC_ARRAY__. If it is not the correct type a type error is returned in the
 * __err__ value.
 *
 * @param err structure to pass the status or an error back in
 * @param val __ojcVal__ to get the members count from
 * @return the number of member values of the __val__ argument.
 */
extern int		ojc_member_count(ojcErr err, ojcVal val);

/**
 * Members of a __OJC_OBJECT__ have keys associated with them. This function
 * returns __true__ if the __val__ is a member of an object and thus has a key.
 *
 * @param val __ojcVal__ to check for a key.
 * @return true if a member of a __OJC_OBJECT__ value and false otherwise.
 */
extern bool		ojc_has_key(ojcVal val);

/**
 * If a __ojcVal__ has a key this function returns that key. If it has no key
 * then NULL is returned.
 *
 * @param val __ojcVal__ to get the key from
 * @return the key associated with the __val__.
 */
extern const char*	ojc_key(ojcVal val);

/**
 * Sets the __ojcVal__ key.
 *
 * @param val __ojcVal__ to set the key on.
 * @param key value to set as the key
 */
extern void		ojc_set_key(ojcVal val, const char *key);

/**
 * Creates a JSON object value.
 *
 * @return the new value.
 */
extern ojcVal		ojc_create_object(void);

/**
 * Creates a JSON array value.
 *
 * @return the new value.
 */
extern ojcVal		ojc_create_array(void);

/**
 * Creates a string value.
 *
 * @param str value of the new instance
 * @param len length of str or <0 to calculate
 * @return the new value.
 */
extern ojcVal		ojc_create_str(const char *str, size_t len);

/**
 * Creates a word value.
 *
 * @param str value of the new instance
 * @param len length of str or <0 to calculate
 * @return the new value.
 */
extern ojcVal		ojc_create_word(const char *str, size_t len);

/**
 * Creates a integer value.
 *
 * @param num value of the new instance
 * @return the new value.
 */
extern ojcVal		ojc_create_int(int64_t num);

/**
 * Creates a double value.
 *
 * @param num value of the new instance
 * @return the new value.
 */
extern ojcVal		ojc_create_double(double num);

/**
 * Creates a number value that is represented as a string. This value type is
 * created when a number is encountered in a JSON document that is too large for
 * an int64_t or for a double.
 *
 * @param num value of the new instance
 * @param len length of str or <0 to calculate
 * @return the new value.
 */
extern ojcVal		ojc_create_number(const char *num, size_t len);

/**
 * Creates a null value.
 *
 * @return the new value.
 */
extern ojcVal		ojc_create_null(void);

/**
 * Creates a boolean value or either __true__ or __false__.
 *
 * @param num value of the new instance
 * @return the new value.
 */
extern ojcVal		ojc_create_bool(bool boo);

/**
 * Appends a __val__ to a JSON object using the specified __key__. This does not
 * replace previous key-value pairs with the same key value. The __err__ struct
 * is set if the __object__ is not an __OJC__OBJECT__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to append __val__ to
 * @param key the key to use in the key-value pair
 * @param val value to append
 */
extern void		ojc_object_append(ojcErr err, ojcVal object, const char *key, ojcVal val);

/**
 * Appends a __val__ to a JSON object using the specified __key__. This does not
 * replace previous key-value pairs with the same key value. The __err__ struct
 * is set if the __object__ is not an __OJC__OBJECT__ type. The key need not be
 * terminated as the __klen__ determines the size.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to append __val__ to
 * @param key the key to use in the key-value pair
 * @param klen length of the key
 * @param val value to append
 */
extern void		ojc_object_nappend(ojcErr err, ojcVal object, const char *key, int klen, ojcVal val);

/**
 * Replaces a __val__ in a JSON object for the specified __key__. Only the first
 * matching member is replaced. The __err__ struct is set if the __object__ is
 * not an __OJC__OBJECT__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to append __val__ to
 * @param key the key to use in the key-value pair
 * @param val new value
 */
extern void		ojc_object_replace(ojcErr err, ojcVal object, const char *key, ojcVal val);

/**
 * Inserts a __val__ in a JSON object with the specified __key__. The insert is
 * before the position specified. The __err__ struct is set if the __object__ is
 * not an __OJC__OBJECT__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to append __val__ to
 * @param before position to insert the new value
 * @param key the key to use in the key-value pair
 * @param val new value
 */
extern void		ojc_object_insert(ojcErr err, ojcVal object, int before, const char *key, ojcVal val);

/**
 * Removes a member in a JSON object at the specified __pos__. The __err__
 * struct is set if the __object__ is not an __OJC__OBJECT__ type or if there is
 * no member at the position specified.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to remove the member from
 * @param pos zero based position of the member to remove
 */
extern void		ojc_object_remove_by_pos(ojcErr err, ojcVal object, int pos);

/**
 * Removes all members in a JSON object with the specified __key__. The __err__
 * struct is set if the __object__ is not an __OJC__OBJECT__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to remove the member from
 * @param key the key to use in the key-value pair
 */
extern void		ojc_object_remove_by_key(ojcErr err, ojcVal object, const char *key);

/**
 * Get the first __val__ in a JSON object with the specified __key__. The
 * __err__ struct is set if the __object__ is not an __OJC__OBJECT__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to get the member from
 * @param key the key to use in finding the key-value pair
 */
extern ojcVal		ojc_object_get_by_key(ojcErr err, ojcVal object, const char *key);

/**
 * Gets a __val__ member in a JSON object at the specified __pos__. The __err__
 * struct is set if the __object__ is not an __OJC__ARRAY__ or __OJC__OBJECT__
 * type. NULL is returned if __pos__ is out of range.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param object object to get the member from
 * @param pos zero based position of the member to get
 */
extern ojcVal		ojc_get_member(ojcErr err, ojcVal val, int pos);

/**
 * Appends a __val__ to a JSON array. The __err__ struct is set if the
 * __object__ is not an __OJC__ARRAY__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param array array to append __val__ to
 * @param val value to append
 */
extern void		ojc_array_append(ojcErr err, ojcVal array, ojcVal val);

/**
 * Pushes a __val__ to the front of a JSON array. The __err__ struct is set if
 * the __object__ is not an __OJC__ARRAY__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param array array to append __val__ to
 * @param val value to push
 */
extern void		ojc_array_push(ojcErr err, ojcVal array, ojcVal val);

/**
 * Removes and returns a __val__ from the front of a JSON array. The __err__
 * struct is set if the __object__ is not an __OJC__ARRAY__ type.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param array array to append __val__ to
 * @return the first element in an array.
 */
extern ojcVal		ojc_array_pop(ojcErr err, ojcVal array);

/**
 * Converts __val__ to a JSON formated string. The __indent__ argument is used
 * indentation of elements in the output. The caller is expected to free the
 * returned string.
 *
 * @param val __ojcVal__ to convert to JSON
 * @param indent indentaiton in spaces to use
 * @return JSON string represented by the __val__.
 */
extern char*		ojc_to_str(ojcVal val, int indent);

/**
 * Converts __val__ to a JSON formated string in the buffer provided. The
 * __indent__ argument is used indentation of elements in the output.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param val __ojcVal__ to convert to JSON
 * @param indent indentation in spaces to use
 * @param buf char buffer to place the output in
 * @param len size of the buffer
 */
extern int		ojc_fill(ojcErr err, ojcVal val, int indent, char *buf, size_t len);

/**
 * Converts __val__ to a JSON formated string and streams the output to a
 * socket. The __indent__ argument is used indentation of elements in the
 * output.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param val __ojcVal__ to convert to JSON
 * @param indent indentation in spaces to use
 * @param socket socket to write to
 */
extern void		ojc_write(ojcErr err, ojcVal val, int indent, int socket);

/**
 * Converts __val__ to a JSON formated string and streams the output to a
 * file. The __indent__ argument is used indentation of elements in the output.
 *
 * @param err pointer to an initialized __ojcErr__ struct.
 * @param val __ojcVal__ to convert to JSON
 * @param indent indentation in spaces to use
 * @param file file to write to
 */
extern void		ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file);

/**
 * Returns a string representation of a type.
 *
 * @param type type to return a string for
 * @return string representation.
 */
extern const char*	ojc_type_str(ojcValType type);

/**
 * Returns a string representation of an error.
 *
 * @param code error code to return a string for
 * @return string representation of an error code.
 */
extern const char*	ojc_error_str(ojcErrCode code);

/**
 * Initializes an __ojcErr__ struct to __OJC_OK__ with no message.
 *
 * @param err struct to initialize
 */
static inline void
ojc_err_init(ojcErr err) {
    err->code = OJC_OK;
    *err->msg = '\0';
}

#ifdef __cplusplus
}
#endif
#endif /* __OJC_H__ */
