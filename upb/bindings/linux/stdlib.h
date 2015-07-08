/*
** Linux-kernel implementations of some stdlib.h functions.
*/

#include <linux/slab.h>

#ifndef UPB_LINUX_STDLIB_H
#define UPB_LINUX_STDLIB_H

static inline void *malloc(size_t size) { return kmalloc(size, GFP_ATOMIC); }
static inline void free(void *p) { kfree(p); }

static inline void *realloc(void *p, size_t size) {
  return krealloc(p, size, GFP_ATOMIC);
}

#endif
