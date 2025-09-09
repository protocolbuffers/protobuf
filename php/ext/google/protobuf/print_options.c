// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "print_options.h"

#include "php.h"

zend_class_entry* options_ce;

void PrintOptions_ModuleInit() {
  zend_class_entry tmp_ce;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\PrintOptions", NULL);
  options_ce = zend_register_internal_class(&tmp_ce);

  // Define constants
  zend_declare_class_constant_long(options_ce, "PRESERVE_PROTO_FIELD_NAMES",
                                   sizeof("PRESERVE_PROTO_FIELD_NAMES") - 1,
                                   PRESERVE_PROTO_FIELD_NAMES);
  zend_declare_class_constant_long(options_ce, "ALWAYS_PRINT_ENUMS_AS_INTS",
                                   sizeof("ALWAYS_PRINT_ENUMS_AS_INTS") - 1,
                                   ALWAYS_PRINT_ENUMS_AS_INTS);
}
