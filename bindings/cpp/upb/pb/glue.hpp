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

bool LoadDescriptorFileIntoSymtab(SymbolTable* s, const char *fname,
                                  Status* status) {
  return upb_load_descriptor_file_into_symtab(s, fname, status);
}

}  // namespace upb

#endif
