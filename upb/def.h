/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

#include "google/protobuf/descriptor.upb.h"
#include "upb/internal/table.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct upb_EnumDef;
typedef struct upb_EnumDef upb_EnumDef;
struct upb_EnumValueDef;
typedef struct upb_EnumValueDef upb_EnumValueDef;
struct upb_ExtensionRange;
typedef struct upb_ExtensionRange upb_ExtensionRange;
struct upb_FieldDef;
typedef struct upb_FieldDef upb_FieldDef;
struct upb_FileDef;
typedef struct upb_FileDef upb_FileDef;
struct upb_MethodDef;
typedef struct upb_MethodDef upb_MethodDef;
struct upb_MessageDef;
typedef struct upb_MessageDef upb_MessageDef;
struct upb_OneofDef;
typedef struct upb_OneofDef upb_OneofDef;
struct upb_ServiceDef;
typedef struct upb_ServiceDef upb_ServiceDef;
struct upb_streamdef;
typedef struct upb_streamdef upb_streamdef;
struct upb_DefPool;
typedef struct upb_DefPool upb_DefPool;

typedef enum { kUpb_Syntax_Proto2 = 2, kUpb_Syntax_Proto3 = 3 } upb_Syntax;

/* All the different kind of well known type messages. For simplicity of check,
 * number wrappers and string wrappers are grouped together. Make sure the
 * order and merber of these groups are not changed.
 */
typedef enum {
  kUpb_WellKnown_Unspecified,
  kUpb_WellKnown_Any,
  kUpb_WellKnown_FieldMask,
  kUpb_WellKnown_Duration,
  kUpb_WellKnown_Timestamp,
  /* number wrappers */
  kUpb_WellKnown_DoubleValue,
  kUpb_WellKnown_FloatValue,
  kUpb_WellKnown_Int64Value,
  kUpb_WellKnown_UInt64Value,
  kUpb_WellKnown_Int32Value,
  kUpb_WellKnown_UInt32Value,
  /* string wrappers */
  kUpb_WellKnown_StringValue,
  kUpb_WellKnown_BytesValue,
  kUpb_WellKnown_BoolValue,
  kUpb_WellKnown_Value,
  kUpb_WellKnown_ListValue,
  kUpb_WellKnown_Struct
} upb_WellKnown;

/* upb_FieldDef ***************************************************************/

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define kUpb_MaxFieldNumber ((1 << 29) - 1)

const google_protobuf_FieldOptions* upb_FieldDef_Options(const upb_FieldDef* f);
bool upb_FieldDef_HasOptions(const upb_FieldDef* f);
const char* upb_FieldDef_FullName(const upb_FieldDef* f);
upb_CType upb_FieldDef_CType(const upb_FieldDef* f);
upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f);
upb_Label upb_FieldDef_Label(const upb_FieldDef* f);
uint32_t upb_FieldDef_Number(const upb_FieldDef* f);
const char* upb_FieldDef_Name(const upb_FieldDef* f);
const char* upb_FieldDef_JsonName(const upb_FieldDef* f);
bool upb_FieldDef_HasJsonName(const upb_FieldDef* f);
bool upb_FieldDef_IsExtension(const upb_FieldDef* f);
bool upb_FieldDef_IsPacked(const upb_FieldDef* f);
const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f);
uint32_t upb_FieldDef_Index(const upb_FieldDef* f);
bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f);
bool upb_FieldDef_IsString(const upb_FieldDef* f);
bool upb_FieldDef_IsOptional(const upb_FieldDef* f);
bool upb_FieldDef_IsRequired(const upb_FieldDef* f);
bool upb_FieldDef_IsRepeated(const upb_FieldDef* f);
bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f);
bool upb_FieldDef_IsMap(const upb_FieldDef* f);
bool upb_FieldDef_HasDefault(const upb_FieldDef* f);
bool upb_FieldDef_HasSubDef(const upb_FieldDef* f);
bool upb_FieldDef_HasPresence(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f);
const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f);
const upb_MiniTable_Field* upb_FieldDef_MiniTable(const upb_FieldDef* f);
const upb_MiniTable_Extension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f);
bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f);

/* upb_OneofDef ***************************************************************/

const google_protobuf_OneofOptions* upb_OneofDef_Options(const upb_OneofDef* o);
bool upb_OneofDef_HasOptions(const upb_OneofDef* o);
const char* upb_OneofDef_Name(const upb_OneofDef* o);
const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o);
uint32_t upb_OneofDef_Index(const upb_OneofDef* o);
bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o);
int upb_OneofDef_FieldCount(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i);

/* Oneof lookups:
 * - ntof:  look up a field by name.
 * - ntofz: look up a field by name (as a null-terminated string).
 * - itof:  look up a field by number. */
const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t length);
UPB_INLINE const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                                       const char* name) {
  return upb_OneofDef_LookupNameWithSize(o, name, strlen(name));
}
const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num);

/* upb_MessageDef *************************************************************/

/* Well-known field tag numbers for map-entry messages. */
#define kUpb_MapEntry_KeyFieldNumber 1
#define kUpb_MapEntry_ValueFieldNumber 2

/* Well-known field tag numbers for Any messages. */
#define kUpb_Any_TypeFieldNumber 1
#define kUpb_Any_ValueFieldNumber 2

/* Well-known field tag numbers for duration messages. */
#define kUpb_Duration_SecondsFieldNumber 1
#define kUpb_Duration_NanosFieldNumber 2

/* Well-known field tag numbers for timestamp messages. */
#define kUpb_Timestamp_SecondsFieldNumber 1
#define kUpb_Timestamp_NanosFieldNumber 2

const google_protobuf_MessageOptions* upb_MessageDef_Options(
    const upb_MessageDef* m);
bool upb_MessageDef_HasOptions(const upb_MessageDef* m);
const char* upb_MessageDef_FullName(const upb_MessageDef* m);
const upb_FileDef* upb_MessageDef_File(const upb_MessageDef* m);
const upb_MessageDef* upb_MessageDef_ContainingType(const upb_MessageDef* m);
const char* upb_MessageDef_Name(const upb_MessageDef* m);
upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m);
upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m);
int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m);
int upb_MessageDef_FieldCount(const upb_MessageDef* m);
int upb_MessageDef_OneofCount(const upb_MessageDef* m);
const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i);
const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i);
const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i);
const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i);
const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m);

UPB_INLINE const upb_OneofDef* upb_MessageDef_FindOneofByName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindOneofByNameWithSize(m, name, strlen(name));
}

UPB_INLINE const upb_FieldDef* upb_MessageDef_FindFieldByName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindFieldByNameWithSize(m, name, strlen(name));
}

UPB_INLINE bool upb_MessageDef_IsMapEntry(const upb_MessageDef* m) {
  return google_protobuf_MessageOptions_map_entry(upb_MessageDef_Options(m));
}

UPB_INLINE bool upb_MessageDef_IsMessageSet(const upb_MessageDef* m) {
  return google_protobuf_MessageOptions_message_set_wire_format(
      upb_MessageDef_Options(m));
}

/* Nested entities. */
int upb_MessageDef_NestedMessageCount(const upb_MessageDef* m);
int upb_MessageDef_NestedEnumCount(const upb_MessageDef* m);
int upb_MessageDef_NestedExtensionCount(const upb_MessageDef* m);
const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i);
const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i);
const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i);

/* Lookup of either field or oneof by name.  Returns whether either was found.
 * If the return is true, then the found def will be set, and the non-found
 * one set to NULL. */
bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** f,
                                       const upb_OneofDef** o);

UPB_INLINE bool upb_MessageDef_FindByName(const upb_MessageDef* m,
                                          const char* name,
                                          const upb_FieldDef** f,
                                          const upb_OneofDef** o) {
  return upb_MessageDef_FindByNameWithSize(m, name, strlen(name), f, o);
}

/* Returns a field by either JSON name or regular proto name. */
const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
UPB_INLINE const upb_FieldDef* upb_MessageDef_FindByJsonName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindByJsonNameWithSize(m, name, strlen(name));
}

/* upb_ExtensionRange *********************************************************/

const google_protobuf_ExtensionRangeOptions* upb_ExtensionRange_Options(
    const upb_ExtensionRange* r);
bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r);
int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* r);
int32_t upb_ExtensionRange_End(const upb_ExtensionRange* r);

/* upb_EnumDef ****************************************************************/

const google_protobuf_EnumOptions* upb_EnumDef_Options(const upb_EnumDef* e);
bool upb_EnumDef_HasOptions(const upb_EnumDef* e);
const char* upb_EnumDef_FullName(const upb_EnumDef* e);
const char* upb_EnumDef_Name(const upb_EnumDef* e);
const upb_FileDef* upb_EnumDef_File(const upb_EnumDef* e);
const upb_MessageDef* upb_EnumDef_ContainingType(const upb_EnumDef* e);
int32_t upb_EnumDef_Default(const upb_EnumDef* e);
int upb_EnumDef_ValueCount(const upb_EnumDef* e);
const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i);

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t len);
const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num);
bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num);

// Convenience wrapper.
UPB_INLINE const upb_EnumValueDef* upb_EnumDef_FindValueByName(
    const upb_EnumDef* e, const char* name) {
  return upb_EnumDef_FindValueByNameWithSize(e, name, strlen(name));
}

/* upb_EnumValueDef ***********************************************************/

const google_protobuf_EnumValueOptions* upb_EnumValueDef_Options(
    const upb_EnumValueDef* e);
bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* e);
const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* e);
const char* upb_EnumValueDef_Name(const upb_EnumValueDef* e);
int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* e);
uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* e);
const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* e);

/* upb_FileDef ****************************************************************/

const google_protobuf_FileOptions* upb_FileDef_Options(const upb_FileDef* f);
bool upb_FileDef_HasOptions(const upb_FileDef* f);
const char* upb_FileDef_Name(const upb_FileDef* f);
const char* upb_FileDef_Package(const upb_FileDef* f);
upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f);
int upb_FileDef_DependencyCount(const upb_FileDef* f);
int upb_FileDef_PublicDependencyCount(const upb_FileDef* f);
int upb_FileDef_WeakDependencyCount(const upb_FileDef* f);
int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f);
int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f);
int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f);
int upb_FileDef_ServiceCount(const upb_FileDef* f);
const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i);
const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i);
const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i);
const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i);
const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i);
const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i);
const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i);
const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f);
const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f);
const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f);

/* upb_MethodDef **************************************************************/

const google_protobuf_MethodOptions* upb_MethodDef_Options(
    const upb_MethodDef* m);
bool upb_MethodDef_HasOptions(const upb_MethodDef* m);
const char* upb_MethodDef_FullName(const upb_MethodDef* m);
int upb_MethodDef_Index(const upb_MethodDef* m);
const char* upb_MethodDef_Name(const upb_MethodDef* m);
const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m);
bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m);
bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m);

/* upb_ServiceDef *************************************************************/

const google_protobuf_ServiceOptions* upb_ServiceDef_Options(
    const upb_ServiceDef* s);
bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s);
const char* upb_ServiceDef_FullName(const upb_ServiceDef* s);
const char* upb_ServiceDef_Name(const upb_ServiceDef* s);
int upb_ServiceDef_Index(const upb_ServiceDef* s);
const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s);
int upb_ServiceDef_MethodCount(const upb_ServiceDef* s);
const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i);
const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name);

/* upb_DefPool ****************************************************************/

upb_DefPool* upb_DefPool_New(void);
void upb_DefPool_Free(upb_DefPool* s);
const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym);
const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);
const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym);
const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym);
const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym);
const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);
const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name);
const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name);
const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);
const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name);
const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len);
const upb_FileDef* upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file,
    upb_Status* status);
size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s);
upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s);
const upb_FieldDef* _upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTable_Extension* ext);
const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum);
const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s);
const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count);

/* For generated code only: loads a generated descriptor. */
typedef struct _upb_DefPool_Init {
  struct _upb_DefPool_Init** deps; /* Dependencies of this file. */
  const upb_MiniTable_File* layout;
  const char* filename;
  upb_StringView descriptor; /* Serialized descriptor. */
} _upb_DefPool_Init;

// Should only be directly called by tests.  This variant lets us suppress
// the use of compiled-in tables, forcing a rebuild of the tables at runtime.
bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable);

UPB_INLINE bool _upb_DefPool_LoadDefInit(upb_DefPool* s,
                                         const _upb_DefPool_Init* init) {
  return _upb_DefPool_LoadDefInitEx(s, init, false);
}

#include "upb/port_undef.inc"

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* UPB_DEF_H_ */
