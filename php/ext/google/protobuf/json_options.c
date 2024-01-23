// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "json_options.h"

#include <php.h>

zend_class_entry* json_options_ce;

/**
 * Message_ModuleInit()
 *
 * Called when the C extension is loaded to register all types.
 */
void JsonOptions_ModuleInit() {
  zend_class_entry tmp_ce;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\JsonSerializeOptions", NULL);

  json_options_ce = zend_register_internal_class(&tmp_ce);
  zend_declare_class_constant_string(
    json_options_ce,
    JSON_SERIALIZE_OPTIONS_EMIT_DEFAULTS,
    strlen(JSON_SERIALIZE_OPTIONS_EMIT_DEFAULTS),
    JSON_SERIALIZE_KEY_EMIT_DEFAULTS
  );

  zend_declare_class_constant_string(
    json_options_ce,
    JSON_SERIALIZE_OPTIONS_PRESERVE_PROTO_FIELD_NAMES,
    strlen(JSON_SERIALIZE_OPTIONS_PRESERVE_PROTO_FIELD_NAMES) ,
    JSON_SERIALIZE_KEY_PRESERVE_PROTO_FIELD_NAMES
  );
}
