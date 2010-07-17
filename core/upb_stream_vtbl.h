/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * vtable declarations for types that are implementing any of the src or sink
 * interfaces.  Only components that are implementing these interfaces need
 * to worry about this file.
 *
 * This is tedious; this is the place in upb where I most wish I had a C++
 * feature.  In C++ the compiler would generate this all for me.  If there's
 * any consolation, it's that I have a bit of flexibility you don't have in
 * C++: I could, with preprocessor magic alone "de-virtualize" this interface
 * for a particular source file.  Say I had a C file that called a upb_src,
 * but didn't want to pay the virtual function overhead.  I could define:
 *
 *   #define upb_src_getdef(src) upb_decoder_getdef((upb_decoder*)src)
 *   #define upb_src_stargmsg(src) upb_decoder_startmsg(upb_decoder*)src)
 *   // etc.
 *
 * The source file is compatible with the regular upb_src interface, but here
 * we bind it to a particular upb_src (upb_decoder), which could lead to
 * improved performance at a loss of flexibility for this one upb_src client.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_SRCSINK_VTBL_H_
#define UPB_SRCSINK_VTBL_H_

#include "upb.h"

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

// upb_src.
typedef struct _upb_fielddef *(*upb_src_getdef_fptr)(upb_src *src);
typedef bool (*upb_src_getval_fptr)(upb_src *src, upb_valueptr val);
typedef bool (*upb_src_getstr_fptr)(upb_src *src, upb_string *str);
typedef bool (*upb_src_skipval_fptr)(upb_src *src);
typedef bool (*upb_src_startmsg_fptr)(upb_src *src);
typedef bool (*upb_src_endmsg_fptr)(upb_src *src);

// upb_sink.
typedef bool (*upb_sink_putdef_fptr)(upb_sink *sink, struct _upb_fielddef *def);
typedef bool (*upb_sink_putval_fptr)(upb_sink *sink, upb_value val);
typedef bool (*upb_sink_putstr_fptr)(upb_sink *sink, upb_string *str);
typedef bool (*upb_sink_startmsg_fptr)(upb_sink *sink);
typedef bool (*upb_sink_endmsg_fptr)(upb_sink *sink);

// upb_bytesrc.
typedef bool (*upb_bytesrc_get_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t minlen);
typedef bool (*upb_bytesrc_append_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// upb_bytesink.
typedef int32_t (*upb_bytesink_put_fptr)(upb_bytesink *sink, upb_string *str);

// Vtables for the above interfaces.
typedef struct {
  upb_src_getdef_fptr   getdef;
  upb_src_getval_fptr   getval;
  upb_src_getstr_fptr   getstr;
  upb_src_skipval_fptr  skipval;
  upb_src_startmsg_fptr startmsg;
  upb_src_endmsg_fptr   endmsg;
} upb_src_vtable;

typedef struct {
  upb_sink_putdef_fptr   putdef;
  upb_sink_putval_fptr   putval;
  upb_sink_putstr_fptr   putstr;
  upb_sink_startmsg_fptr startmsg;
  upb_sink_endmsg_fptr   endmsg;
} upb_sink_vtable;

typedef struct {
  upb_bytesrc_get_fptr     get;
  upb_bytesrc_append_fptr  append;
} upb_bytesrc_vtable;

typedef struct {
  upb_bytesink_put_fptr  put;
} upb_bytesink_vtable;

// "Base Class" definitions; components that implement these interfaces should
// contain one of these structures.

struct upb_src {
  upb_src_vtable *vtbl;
  upb_status status;
  bool eof;
};

struct upb_sink {
  upb_sink_vtable *vtbl;
  upb_status status;
  bool eof;
};

struct upb_bytesrc {
  upb_bytesrc_vtable *vtbl;
  upb_status status;
  bool eof;
};

struct upb_bytesink {
  upb_bytesink_vtable *vtbl;
  upb_status status;
  bool eof;
};

INLINE void upb_src_init(upb_src *s, upb_src_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

INLINE void upb_sink_init(upb_sink *s, upb_sink_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

INLINE void upb_bytesrc_init(upb_bytesrc *s, upb_bytesrc_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

INLINE void upb_bytesink_init(upb_bytesink *s, upb_bytesink_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

// Implementation of virtual function dispatch.
INLINE struct _upb_fielddef *upb_src_getdef(upb_src *src) {
  return src->vtbl->getdef(src);
}
INLINE bool upb_src_getval(upb_src *src, upb_valueptr val) {
  return src->vtbl->getval(src, val);
}
INLINE bool upb_src_getstr(upb_src *src, upb_string *str) {
  return src->vtbl->getstr(src, str);
}
INLINE bool upb_src_skipval(upb_src *src) { return src->vtbl->skipval(src); }
INLINE bool upb_src_startmsg(upb_src *src) { return src->vtbl->startmsg(src); }
INLINE bool upb_src_endmsg(upb_src *src) { return src->vtbl->endmsg(src); }

// Implementation of type-specific upb_src accessors.  If we encounter a upb_src
// where these can be implemented directly in a measurably more efficient way,
// we can make these part of the vtable also.
//
// For <64-bit types we have to use a temporary to accommodate baredecoder,
// which does not know the actual width of the type.
INLINE bool upb_src_getbool(upb_src *src, bool *_bool) {
  upb_value val;
  bool ret = upb_src_getval(src, upb_value_addrof(&val));
  *_bool = val._bool;
  return ret;
}

INLINE bool upb_src_getint32(upb_src *src, int32_t *i32) {
  upb_value val;
  bool ret = upb_src_getval(src, upb_value_addrof(&val));
  *i32 = val.int32;
  return ret;
}

// TODO.
bool upb_src_getint32(upb_src *src, int32_t *val);
bool upb_src_getint64(upb_src *src, int64_t *val);
bool upb_src_getuint32(upb_src *src, uint32_t *val);
bool upb_src_getuint64(upb_src *src, uint64_t *val);
bool upb_src_getfloat(upb_src *src, float *val);
bool upb_src_getdouble(upb_src *src, double *val);

// upb_bytesrc
INLINE bool upb_bytesrc_get(
    upb_bytesrc *bytesrc, upb_string *str, upb_strlen_t minlen) {
  return bytesrc->vtbl->get(bytesrc, str, minlen);
}

INLINE bool upb_bytesrc_append(
    upb_bytesrc *bytesrc, upb_string *str, upb_strlen_t len) {
  return bytesrc->vtbl->append(bytesrc, str, len);
}

// upb_sink
INLINE bool upb_sink_putdef(upb_sink *sink, struct _upb_fielddef *def) {
  return sink->vtbl->putdef(sink, def);
}
INLINE bool upb_sink_putval(upb_sink *sink, upb_value val) {
  return sink->vtbl->putval(sink, val);
}
INLINE bool upb_sink_putstr(upb_sink *sink, upb_string *str) {
  return sink->vtbl->putstr(sink, str);
}
INLINE bool upb_sink_startmsg(upb_sink *sink) {
  return sink->vtbl->startmsg(sink);
}
INLINE bool upb_sink_endmsg(upb_sink *sink) {
  return sink->vtbl->endmsg(sink);
}

INLINE upb_status *upb_sink_status(upb_sink *sink) { return &sink->status; }

// upb_bytesink
INLINE int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str) {
  return sink->vtbl->put(sink, str);
}
INLINE upb_status *upb_bytesink_status(upb_bytesink *sink) {
  return &sink->status;
}

// upb_bytesink


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
