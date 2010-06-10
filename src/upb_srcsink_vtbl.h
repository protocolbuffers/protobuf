/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * vtable declarations for types that are implementing any of the src or sink
 * interfaces.  Only components that are implementing these interfaces need
 * to worry about this file.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_SRCSINK_VTBL_H_
#define UPB_SRCSINK_VTBL_H_

#include "upb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_src;
typedef struct upb_src upb_src;
struct upb_sink;
typedef struct upb_sink upb_sink;
struct upb_bytesrc;
typedef struct upb_bytesrc upb_bytesrc;
struct upb_bytesink;
typedef struct upb_bytesink upb_bytesink;

// Typedefs for function pointers to all of the virtual functions.
typedef upb_fielddef (*upb_src_getdef_fptr)(upb_src *src);
typedef bool (*upb_src_getval_fptr)(upb_src *src, upb_valueptr val);
typedef bool (*upb_src_skipval_fptr)(upb_src *src);
typedef bool (*upb_src_startmsg_fptr)(upb_src *src);
typedef bool (*upb_src_endmsg_fptr)(upb_src *src);

typedef bool (*upb_sink_putdef_fptr)(upb_sink *sink, upb_fielddef *def);
typedef bool (*upb_sink_putval_fptr)(upb_sink *sink, upb_value val);
typedef bool (*upb_sink_startmsg_fptr)(upb_sink *sink);
typedef bool (*upb_sink_endmsg_fptr)(upb_sink *sink);

typedef upb_string *(*upb_bytesrc_get_fptr)(upb_bytesrc *src);
typedef void (*upb_bytesrc_recycle_fptr)(upb_bytesrc *src, upb_string *str);
typedef bool (*upb_bytesrc_append_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t len);

typedef int32_t (*upb_bytesink_put_fptr)(upb_bytesink *sink, upb_string *str);

// Vtables for the above interfaces.
typedef struct {
  upb_src_getdef_fptr   getdef;
  upb_src_getval_fptr   getval;
  upb_src_skipval_fptr  skipval;
  upb_src_startmsg_fptr startmsg;
  upb_src_endmsg_fptr   endmsg;
} upb_src_vtable;

typedef struct {
  upb_bytesrc_get_fptr     get;
  upb_bytesrc_append_fptr  append;
  upb_bytesrc_recycle_fptr recycle;
} upb_bytesrc_vtable;

// "Base Class" definitions; components that implement these interfaces should
// contain one of these structures.

struct upb_src {
  upb_src_vtable *vtbl;
  upb_status status;
  bool eof;
#ifndef NDEBUG
  int state;  // For debug-mode checking of API usage.
#endif
};

struct upb_bytesrc {
  upb_bytesrc_vtable *vtbl;
  upb_status status;
  bool eof;
};

INLINE void upb_src_init(upb_src *s, upb_src_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
#ifndef DEBUG
  // TODO: initialize debug-mode checking.
#endif
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
