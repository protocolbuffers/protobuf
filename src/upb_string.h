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

#include <stdlib.h>
#include <string.h>
#include "upb.h"

struct upb_string {
  /* We expect the data to be 8-bit clean (uint8_t), but char* is such an
   * ingrained convention that we follow it. */
  char *ptr;
  uint32_t byte_len;
};

INLINE bool upb_streql(struct upb_string *s1, struct upb_string *s2) {
  return s1->byte_len == s2->byte_len &&
         memcmp(s1->ptr, s2->ptr, s1->byte_len) == 0;
}

INLINE int upb_strcmp(struct upb_string s1, struct upb_string s2) {
  size_t common_length = min(s1.byte_len, s2.byte_len);
  int common_diff = memcmp(s1.ptr, s2.ptr, common_length);
  if(common_diff == 0) return s1.byte_len - s2.byte_len;
  else return common_diff;
}

INLINE void upb_strcpy(struct upb_string *dest, struct upb_string *src) {
  memcpy(dest->ptr, src->ptr, src->byte_len);
  dest->byte_len = src->byte_len;
}

INLINE struct upb_string upb_strdup(struct upb_string s) {
  struct upb_string copy;
  copy.ptr = (char*)malloc(s.byte_len);
  copy.byte_len = s.byte_len;
  memcpy(copy.ptr, s.ptr, s.byte_len);
  return copy;
}

INLINE struct upb_string upb_strdupc(char *s) {
  struct upb_string copy;
  copy.byte_len = strlen(s);
  copy.ptr = (char*)malloc(copy.byte_len);
  memcpy(copy.ptr, s, copy.byte_len);
  return copy;
}

INLINE void upb_strfree(struct upb_string s) {
  free(s.ptr);
}

/* Reads an entire file into a newly-allocated string. */
bool upb_strreadfile(const char *filename, struct upb_string *data);

/* Allows defining upb_strings as literals, ie:
 *   struct upb_string str = UPB_STRLIT("Hello, World!\n");
 * Doesn't work with C++ due to lack of struct initializer syntax.
 */
#define UPB_STRLIT(strlit) {.ptr=strlit, .byte_len=sizeof(strlit)-1}

/* Allows using upb_strings in printf, ie:
 *   struct upb_string str = UPB_STRLIT("Hello, World!\n");
 *   printf("String is: " UPB_STRFMT, UPB_STRARG(str)); */
#define UPB_STRARG(str) (str).byte_len, (str).ptr
#define UPB_STRFMT "%.*s"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
