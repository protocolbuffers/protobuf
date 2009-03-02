/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stddef.h>
#include "pbstruct.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define ALIGN_UP(p, t) (alignof(t) + ((p - 1) & ~(alignof(t) - 1)))

