// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/oneof_def.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/field_def.h"
#include "upb/reflection/internal/message_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_OneofDef {
  UPB_ALIGN_AS(8) const UPB_DESC(OneofOptions*) opts;
  const UPB_DESC(FeatureSet*) resolved_features;
  const upb_MessageDef* parent;
  const char* full_name;
  int field_count;
  bool synthetic;
  const upb_FieldDef** fields;
  upb_strtable ntof;  // lookup a field by name
  upb_inttable itof;  // lookup a field by number (index)
};

upb_OneofDef* _upb_OneofDef_At(const upb_OneofDef* o, int i) {
  return (upb_OneofDef*)&o[i];
}

const UPB_DESC(OneofOptions) * upb_OneofDef_Options(const upb_OneofDef* o) {
  return o->opts;
}

bool upb_OneofDef_HasOptions(const upb_OneofDef* o) {
  return o->opts != (void*)kUpbDefOptDefault;
}

const UPB_DESC(FeatureSet) *
    upb_OneofDef_ResolvedFeatures(const upb_OneofDef* o) {
  return o->resolved_features;
}

const char* upb_OneofDef_FullName(const upb_OneofDef* o) {
  return o->full_name;
}

const char* upb_OneofDef_Name(const upb_OneofDef* o) {
  return _upb_DefBuilder_FullToShort(o->full_name);
}

const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o) {
  return o->parent;
}

int upb_OneofDef_FieldCount(const upb_OneofDef* o) { return o->field_count; }

const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i) {
  UPB_ASSERT(i < o->field_count);
  return o->fields[i];
}

int upb_OneofDef_numfields(const upb_OneofDef* o) { return o->field_count; }

uint32_t upb_OneofDef_Index(const upb_OneofDef* o) {
  // Compute index in our parent's array.
  return o - upb_MessageDef_Oneof(o->parent, 0);
}

bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o) { return o->synthetic; }

const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t size) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, size, &val)
             ? upb_value_getptr(val)
             : NULL;
}

const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                            const char* name) {
  return upb_OneofDef_LookupNameWithSize(o, name, strlen(name));
}

const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num) {
  upb_value val;
  return upb_inttable_lookup(&o->itof, num, &val) ? upb_value_getptr(val)
                                                  : NULL;
}

void _upb_OneofDef_Insert(upb_DefBuilder* ctx, upb_OneofDef* o,
                          const upb_FieldDef* f, const char* name,
                          size_t size) {
  o->field_count++;
  if (_upb_FieldDef_IsProto3Optional(f)) o->synthetic = true;

  const int number = upb_FieldDef_Number(f);
  const upb_value v = upb_value_constptr(f);

  // TODO: This lookup is unfortunate because we also perform it when
  // inserting into the message's table. Unfortunately that step occurs after
  // this one and moving things around could be tricky so let's leave it for
  // a future refactoring.
  const bool number_exists = upb_inttable_lookup(&o->itof, number, NULL);
  if (UPB_UNLIKELY(number_exists)) {
    _upb_DefBuilder_Errf(ctx, "oneof fields have the same number (%d)", number);
  }

  // TODO: More redundant work happening here.
  const bool name_exists = upb_strtable_lookup2(&o->ntof, name, size, NULL);
  if (UPB_UNLIKELY(name_exists)) {
    _upb_DefBuilder_Errf(ctx, "oneof fields have the same name (%.*s)",
                         (int)size, name);
  }

  const bool ok = upb_inttable_insert(&o->itof, number, v, ctx->arena) &&
                  upb_strtable_insert(&o->ntof, name, size, v, ctx->arena);
  if (UPB_UNLIKELY(!ok)) {
    _upb_DefBuilder_OomErr(ctx);
  }
}

// Returns the synthetic count.
size_t _upb_OneofDefs_Finalize(upb_DefBuilder* ctx, upb_MessageDef* m) {
  int synthetic_count = 0;

  for (int i = 0; i < upb_MessageDef_OneofCount(m); i++) {
    upb_OneofDef* o = (upb_OneofDef*)upb_MessageDef_Oneof(m, i);

    if (o->synthetic && o->field_count != 1) {
      _upb_DefBuilder_Errf(ctx,
                           "Synthetic oneofs must have one field, not %d: %s",
                           o->field_count, upb_OneofDef_Name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      _upb_DefBuilder_Errf(
          ctx, "Synthetic oneofs must be after all other oneofs: %s",
          upb_OneofDef_Name(o));
    }

    o->fields =
        UPB_DEFBUILDER_ALLOCARRAY(ctx, const upb_FieldDef*, o->field_count);
    o->field_count = 0;
  }

  for (int i = 0; i < upb_MessageDef_FieldCount(m); i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_OneofDef* o = (upb_OneofDef*)upb_FieldDef_ContainingOneof(f);
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  return synthetic_count;
}

static void create_oneofdef(upb_DefBuilder* ctx, upb_MessageDef* m,
                            const UPB_DESC(OneofDescriptorProto*) oneof_proto,
                            const UPB_DESC(FeatureSet*) parent_features,
                            const upb_OneofDef* _o) {
  upb_OneofDef* o = (upb_OneofDef*)_o;

  UPB_DEF_SET_OPTIONS(o->opts, OneofDescriptorProto, OneofOptions, oneof_proto);
  o->resolved_features = _upb_DefBuilder_ResolveFeatures(
      ctx, parent_features, UPB_DESC(OneofOptions_features)(o->opts));

  upb_StringView name = UPB_DESC(OneofDescriptorProto_name)(oneof_proto);

  o->parent = m;
  o->full_name =
      _upb_DefBuilder_MakeFullName(ctx, upb_MessageDef_FullName(m), name);
  o->field_count = 0;
  o->synthetic = false;

  if (upb_MessageDef_FindByNameWithSize(m, name.data, name.size, NULL, NULL)) {
    _upb_DefBuilder_Errf(ctx, "duplicate oneof name (%s)", o->full_name);
  }

  upb_value v = _upb_DefType_Pack(o, UPB_DEFTYPE_ONEOF);
  bool ok = _upb_MessageDef_Insert(m, name.data, name.size, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_inttable_init(&o->itof, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_strtable_init(&o->ntof, 4, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

// Allocate and initialize an array of |n| oneof defs.
upb_OneofDef* _upb_OneofDefs_New(upb_DefBuilder* ctx, int n,
                                 const UPB_DESC(OneofDescriptorProto*)
                                     const* protos,
                                 const UPB_DESC(FeatureSet*) parent_features,
                                 upb_MessageDef* m) {
  _upb_DefType_CheckPadding(sizeof(upb_OneofDef));

  upb_OneofDef* o = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_OneofDef, n);
  for (int i = 0; i < n; i++) {
    create_oneofdef(ctx, m, protos[i], parent_features, &o[i]);
  }
  return o;
}
