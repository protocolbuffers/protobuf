/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_PB_GLUE_HPP
#define UPB_PB_GLUE_HPP

#include "upb/upb.hpp"
#include "upb/pb/glue.h"

namespace upb {

// All routines that load descriptors expect the descriptor to be a
// FileDescriptorSet.
bool LoadDescriptorFileIntoSymtab(SymbolTable* s, const char *fname,
                                  Status* status) {
  return upb_load_descriptor_file_into_symtab(s, fname, status);
}

bool LoadDescriptorIntoSymtab(SymbolTable* s, const char* str,
                              size_t len, Status* status) {
  return upb_load_descriptor_into_symtab(s, str, len, status);
}

template <typename T>
bool LoadDescriptorIntoSymtab(SymbolTable* s, const T& desc, Status* status) {
  return upb_load_descriptor_into_symtab(s, desc.c_str(), desc.size(), status);
}

}  // namespace upb

#endif
