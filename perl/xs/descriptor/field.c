#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/field.h"
#include "upb/base/descriptor_constants.h"

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_FieldDef_GetWrapper(pTHX_ const upb_FieldDef *f) {
    RETURN_CACHED_OR_CREATE_BLESSED(f, "Protobuf::Descriptor::Field");
}

const upb_FieldDef* PerlUpb_FieldDef_GetField(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_FieldDef, sv, "Protobuf::Descriptor::Field");
}

SV* PerlUpb_FieldDef_Name(pTHX_ const upb_FieldDef *f) {
    if (!f) return newSV(0);
    const char* name = upb_FieldDef_Name(f);
    return name ? newSVpv(name, 0) : newSV(0);
}

const char* PerlUpb_FieldDef_TypeName(pTHX_ const upb_FieldDef *f) {
    if (!f) return "unknown";
    upb_FieldType type = upb_FieldDef_Type(f);
    switch (type) {
        case kUpb_FieldType_Double:   return "double";
        case kUpb_FieldType_Float:    return "float";
        case kUpb_FieldType_Int64:    return "int64";
        case kUpb_FieldType_UInt64:   return "uint64";
        case kUpb_FieldType_Int32:    return "int32";
        case kUpb_FieldType_Fixed64:  return "fixed64";
        case kUpb_FieldType_Fixed32:  return "fixed32";
        case kUpb_FieldType_Bool:     return "bool";
        case kUpb_FieldType_String:   return "string";
        case kUpb_FieldType_Group:    return "group";
        case kUpb_FieldType_Message:  return "message";
        case kUpb_FieldType_Bytes:    return "bytes";
        case kUpb_FieldType_UInt32:   return "uint32";
        case kUpb_FieldType_Enum:     return "enum";
        case kUpb_FieldType_SFixed32: return "sfixed32";
        case kUpb_FieldType_SFixed64: return "sfixed64";
        case kUpb_FieldType_SInt32:   return "sint32";
        case kUpb_FieldType_SInt64:   return "sint64";
        default: return "unknown";
    }
}

const char* PerlUpb_FieldDef_LabelName(pTHX_ const upb_FieldDef *f) {
    if (!f) return "unknown";
    upb_Label label = upb_FieldDef_Label(f);
    switch (label) {
        case kUpb_Label_Optional: return "optional";
        case kUpb_Label_Required: return "required";
        case kUpb_Label_Repeated: return "repeated";
        default: return "unknown";
    }
}

int PerlUpb_FieldDef_Type(pTHX_ const upb_FieldDef *f) {
    if (!f) return 0;
    return upb_FieldDef_Type(f);
}

int PerlUpb_FieldDef_Label(pTHX_ const upb_FieldDef *f) {
    if (!f) return 0;
    return upb_FieldDef_Label(f);
}

uint32_t PerlUpb_FieldDef_Number(pTHX_ const upb_FieldDef *f) {
    if (!f) return 0;
    return upb_FieldDef_Number(f);
}

bool PerlUpb_FieldDef_IsRepeated(pTHX_ const upb_FieldDef *f) {
    if (!f) return false;
    return upb_FieldDef_IsRepeated(f);
}

bool PerlUpb_FieldDef_IsMap(pTHX_ const upb_FieldDef *f) {
    if (!f) return false;
    return upb_FieldDef_IsMap(f);
}

bool PerlUpb_FieldDef_IsSubMessage(pTHX_ const upb_FieldDef *f) {
    if (!f) return false;
    return upb_FieldDef_IsSubMessage(f);
}

bool PerlUpb_FieldDef_IsExtension(pTHX_ const upb_FieldDef *f) {
    if (!f) return false;
    return upb_FieldDef_IsExtension(f);
}

bool PerlUpb_FieldDef_IsPacked(pTHX_ const upb_FieldDef *f) {
    if (!f) return false;
    return upb_FieldDef_IsPacked(f);
}
