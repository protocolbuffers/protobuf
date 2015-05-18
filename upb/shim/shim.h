/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * For handlers that do very tiny, very simple operations, the function call
 * overhead of calling a handler can be significant.  This file allows the
 * user to define handlers that do something very simple like store the value
 * to memory and/or set a hasbit.  JIT compilers can then special-case these
 * handlers and emit specialized code for them instead of actually calling the
 * handler.
 *
 * The functionality is very simple/limited right now but may expand to be able
 * to call another function.
 */

#ifndef UPB_SHIM_H
#define UPB_SHIM_H

#include "upb/handlers.h"

typedef struct {
  size_t offset;
  int32_t hasbit;
} upb_shim_data;

#ifdef __cplusplus

namespace upb {

struct Shim {
  typedef upb_shim_data Data;

  /* Sets a handler for the given field that writes the value to the given
   * offset and, if hasbit >= 0, sets a bit at the given bit offset.  Returns
   * true if the handler was set successfully. */
  static bool Set(Handlers *h, const FieldDef *f, size_t ofs, int32_t hasbit);

  /* If this handler is a shim, returns the corresponding upb::Shim::Data and
   * stores the type in "type".  Otherwise returns NULL. */
  static const Data* GetData(const Handlers* h, Handlers::Selector s,
                             FieldDef::Type* type);
};

}  /* namespace upb */

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
bool upb_shim_set(upb_handlers *h, const upb_fielddef *f, size_t offset,
                  int32_t hasbit);
const upb_shim_data *upb_shim_getdata(const upb_handlers *h, upb_selector_t s,
                                      upb_fieldtype_t *type);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ Wrappers. */
namespace upb {
inline bool Shim::Set(Handlers* h, const FieldDef* f, size_t ofs,
                      int32_t hasbit) {
  return upb_shim_set(h, f, ofs, hasbit);
}
inline const Shim::Data* Shim::GetData(const Handlers* h, Handlers::Selector s,
                                       FieldDef::Type* type) {
  return upb_shim_getdata(h, s, type);
}
}  /* namespace upb */
#endif

#endif  /* UPB_SHIM_H */
