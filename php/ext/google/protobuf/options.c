// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "php.h"
#include "options.h"

zend_class_entry* options_ce;

void Options_ModuleInit() {
    zend_class_entry tmp_ce;

    INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Options", NULL);
    options_ce = zend_register_internal_class(&tmp_ce);

    // Define constants
    zend_declare_class_constant_long(options_ce, "JSON_ENCODE_EMIT_DEFAULTS", sizeof("JSON_ENCODE_EMIT_DEFAULTS") - 1, JSON_ENCODE_EMIT_DEFAULTS);
    zend_declare_class_constant_long(options_ce, "JSON_ENCODE_FORMAT_ENUMS_AS_INTEGERS", sizeof("JSON_ENCODE_FORMAT_ENUMS_AS_INTEGERS") - 1, JSON_ENCODE_FORMAT_ENUMS_AS_INTEGERS);
    zend_declare_class_constant_long(options_ce, "JSON_ENCODE_PRESERVE_PROTO_FIELDNAMES", sizeof("JSON_ENCODE_PRESERVE_PROTO_FIELDNAMES") - 1, JSON_ENCODE_PRESERVE_PROTO_FIELDNAMES);
}
