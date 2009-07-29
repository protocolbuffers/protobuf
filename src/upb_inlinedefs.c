/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file, if compiled, will contain standalone (non-inlined) versions of
 * all inline functions defined in header files.  We don't generally use this
 * file since we use "static inline" for inline functions (which will put a
 * standalone version of the function in any .o file that needs it, but
 * compiling this file and dumping the object file will let us inspect how
 * inline functions are compiled, so we keep it around.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#define INLINE
#include "upb_array.h"
#include "upb_context.h"
#include "upb_enum.h"
#include "upb_msg.h"
#include "upb_parse.h"
#include "upb_serialize.h"
#include "upb_string.h"
#include "upb_table.h"
#include "upb_text.h"
