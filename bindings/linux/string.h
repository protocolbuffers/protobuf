/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_LINUX_STRING_H_
#define UPB_LINUX_STRING_H_

#include <linux/string.h>
#include <stdlib.h>
#include "upb/upb.h"  // For INLINE.

INLINE char *strdup(const char *s) {
  size_t len = strlen(s);
  char *ret = malloc(len + 1);
  if (ret == NULL) return NULL;
  // Be particularly defensive and guard against buffer overflow if there
  // is a concurrent mutator.
  strncpy(ret, s, len);
  ret[len] = '\0';
  return ret;
}

#endif  /* UPB_DEF_H_ */
