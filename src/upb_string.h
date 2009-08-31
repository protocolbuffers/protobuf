/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.

 * Defines a delimited (as opposed to null-terminated) string type and some
 * library functions for manipulating them.
 *
 * There are two primary reasons upb uses delimited strings.  One is that they
 * can be more efficient for some operations because they do not have to scan
 * the string to find its length.  For example, streql can start by just
 * comparing the lengths (very efficient) and scan the strings themselves only
 * if the lengths are equal.
 *
 * More importantly, using delimited strings makes it possible for strings to
 * reference substrings of other strings.  For example, if I am parsing a
 * protobuf I can create a string that references the original protobuf's
 * string data.  With NULL-termination I would be forced to write a NULL
 * into the middle of the protobuf's data, which is less than ideal and in
 * some cases not practical or possible.
 */

#ifndef UPB_STRING_H_
#define UPB_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb_struct.h"

/* Allocation/Deallocation/Resizing. ******************************************/

INLINE struct upb_string *upb_string_new(void)
{
  struct upb_string *str = (struct upb_string*)malloc(sizeof(*str));
  upb_mmhead_init(&str->mmhead);
  str->ptr = NULL;
  str->byte_len = 0;
  str->byte_size = 0;
  return str;
}

/* For internal use only. */
INLINE void upb_string_destroy(struct upb_string *str)
{
  if(str->byte_size != 0) free(str->ptr);
  free(str);
}

INLINE void upb_string_unref(struct upb_string *str)
{
  if(upb_mmhead_unref(&str->mmhead)) upb_string_destroy(str);
}

/* Resizes the string to size, reallocating if necessary.  Does not preserve
 * existing data. */
INLINE void upb_string_resize(struct upb_string *str, uint32_t size)
{
  if(str->byte_size < size) {
    /* Need to resize. */
    str->byte_size = size;
    void *oldptr = str->byte_size == 0 ? NULL : str->ptr;
    str->ptr = (char*)realloc(oldptr, str->byte_size);
  }
  str->byte_len = size;
}

/* Library functions. *********************************************************/

INLINE bool upb_streql(struct upb_string *s1, struct upb_string *s2) {
  return s1->byte_len == s2->byte_len &&
         memcmp(s1->ptr, s2->ptr, s1->byte_len) == 0;
}

INLINE int upb_strcmp(struct upb_string *s1, struct upb_string *s2) {
  size_t common_length = UPB_MIN(s1->byte_len, s2->byte_len);
  int common_diff = memcmp(s1->ptr, s2->ptr, common_length);
  return common_diff == 0 ? (int)s1->byte_len - (int)s2->byte_len : common_diff;
}

INLINE void upb_strcpy(struct upb_string *dest, struct upb_string *src) {
  dest->byte_len = src->byte_len;
  upb_string_resize(dest, dest->byte_len);
  memcpy(dest->ptr, src->ptr, src->byte_len);
}

INLINE struct upb_string *upb_strdup(struct upb_string *s) {
  struct upb_string *copy = upb_string_new();
  upb_strcpy(copy, s);
  return copy;
}

INLINE struct upb_string *upb_strdupc(const char *s) {
  struct upb_string *copy = upb_string_new();
  copy->byte_len = strlen(s);
  upb_string_resize(copy, copy->byte_len);
  memcpy(copy->ptr, s, copy->byte_len);
  return copy;
}

/* Reads an entire file into a newly-allocated string. */
struct upb_string *upb_strreadfile(const char *filename);

/* Allows defining upb_strings as literals, ie:
 *   struct upb_string str = UPB_STRLIT("Hello, World!\n");
 * Doesn't work with C++ due to lack of struct initializer syntax.
 */
#define UPB_STRLIT(strlit) {.ptr=strlit, .byte_len=sizeof(strlit)-1}

/* Allows using upb_strings in printf, ie:
 *   struct upb_string str = UPB_STRLIT("Hello, World!\n");
 *   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) (str)->byte_len, (str)->ptr
#define UPB_STRFMT "%.*s"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
