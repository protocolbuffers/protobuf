
#include "xs/convert/upb_to_sv.h"
#include "xs/protobuf.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/utils.h"
#include "xs/map/map.h"
#include "xs/repeated/repeated.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "upb/wire/types.h"
#include "upb/message/array.h"
#include <stdint.h>
#include <stdio.h>

static SV* convert_singular_upb_to_sv(pTHX_ const upb_MessageValue *val, const upb_FieldDef *f, SV *parent_arena_sv) {
    upb_FieldType type = upb_FieldDef_Type(f);
    switch (type) {
        case kUpb_FieldType_Int32:
        case kUpb_FieldType_SInt32:
        case kUpb_FieldType_SFixed32:
        case kUpb_FieldType_Enum:
            return newSViv(val->int32_val);
        case kUpb_FieldType_String: {
            SV* sv = newSVpvn(val->str_val.data, val->str_val.size);
            SvUTF8_on(sv);
            return sv;
        }
        case kUpb_FieldType_Bytes: {
            return newSVpvn(val->str_val.data, val->str_val.size);
        }
        case kUpb_FieldType_Bool:
            return newSVsv(val->bool_val ? &PL_sv_yes : &PL_sv_no);
        case kUpb_FieldType_Float:
            return newSVnv((double)val->float_val);
        case kUpb_FieldType_Double:
            return newSVnv(val->double_val);
        case kUpb_FieldType_UInt32:
        case kUpb_FieldType_Fixed32:
            return newSVuv(val->uint32_val);
        case kUpb_FieldType_Int64:
        case kUpb_FieldType_SInt64:
        case kUpb_FieldType_SFixed64:
            return PerlUpb_I64ToSV(aTHX_ val->int64_val);
        case kUpb_FieldType_UInt64:
        case kUpb_FieldType_Fixed64:
            return PerlUpb_U64ToSV(aTHX_ val->uint64_val);
        case kUpb_FieldType_Message:
        case kUpb_FieldType_Group: {
            const upb_MessageDef *mdef = upb_FieldDef_MessageSubDef(f);
            if (!mdef) {
                const char *field_name = upb_FieldDef_Name(f);
                croak("PerlUpb_UpbToSv: Could not get MessageDef for field %s", field_name);
            }
            return PerlUpb_WrapMessage(aTHX_ val->msg_val, mdef, parent_arena_sv, 0);
        }
        default:
            croak("Unknown upb field type: %d", type);
            return newSV(0); // Not reached
    }
}

SV* PerlUpb_UpbToSv_Element(pTHX_ const upb_MessageValue *val, const upb_FieldDef *f, SV *parent_arena_sv) {
    if (!f || !val) return newSV(0);
    return convert_singular_upb_to_sv(aTHX_ val, f, parent_arena_sv);
}

// VPP-style Batch Conversion
void PerlUpb_UpbToSv_Batch(pTHX_ const upb_MessageValue *vals, const upb_FieldDef *f, SV *parent_arena_sv, SV **out, size_t count) {
    // Process in batches of 4 to allow compiler to unroll and potentially use SIMD
    size_t i = 0;
    for (; i + 3 < count; i += 4) {
        // Prefetch logic could go here if we were dealing with larger structures
        out[i]   = convert_singular_upb_to_sv(aTHX_ &vals[i],   f, parent_arena_sv);
        out[i+1] = convert_singular_upb_to_sv(aTHX_ &vals[i+1], f, parent_arena_sv);
        out[i+2] = convert_singular_upb_to_sv(aTHX_ &vals[i+2], f, parent_arena_sv);
        out[i+3] = convert_singular_upb_to_sv(aTHX_ &vals[i+3], f, parent_arena_sv);
    }
    for (; i < count; i++) {
        out[i] = convert_singular_upb_to_sv(aTHX_ &vals[i], f, parent_arena_sv);
    }
}

// Optimized raw batch for scalar types (non-message, non-string)
void PerlUpb_UpbToSv_BatchRaw(pTHX_ const void *data, upb_CType type, SV **out, size_t count) {
    size_t i = 0;
    switch (type) {
        case kUpb_CType_Bool: {
            const bool *src = (const bool*)data;
            for (; i < count; i++) out[i] = newSVsv(src[i] ? &PL_sv_yes : &PL_sv_no);
            break;
        }
        case kUpb_CType_Int32:
        case kUpb_CType_Enum: {
            const int32_t *src = (const int32_t*)data;
            for (; i < count; i++) out[i] = newSViv(src[i]);
            break;
        }
        case kUpb_CType_UInt32: {
            const uint32_t *src = (const uint32_t*)data;
            for (; i < count; i++) out[i] = newSVuv(src[i]);
            break;
        }
        case kUpb_CType_Float: {
            const float *src = (const float*)data;
            for (; i < count; i++) out[i] = newSVnv((double)src[i]);
            break;
        }
        case kUpb_CType_Double: {
            const double *src = (const double*)data;
            for (; i < count; i++) out[i] = newSVnv(src[i]);
            break;
        }
        case kUpb_CType_Int64: {
            const int64_t *src = (const int64_t*)data;
            for (; i < count; i++) out[i] = PerlUpb_I64ToSV(aTHX_ src[i]);
            break;
        }
        case kUpb_CType_UInt64: {
            const uint64_t *src = (const uint64_t*)data;
            for (; i < count; i++) out[i] = PerlUpb_U64ToSV(aTHX_ src[i]);
            break;
        }
        default:
            croak("PerlUpb_UpbToSv_BatchRaw: Unsupported CType %d", type);
    }
}

#include "xs/protobuf/registry.h"

static SV* wrap_container_native(pTHX_ SV* internal_sv, HV* public_stash, const char* wrap_func) {
    HV* internal_hv = (HV*)SvRV(internal_sv);
    SV** public_svp = hv_fetch(internal_hv, "_public", 7, 0);
    if (public_svp && SvOK(*public_svp)) {
        return SvREFCNT_inc(*public_svp);
    }

    if (!get_cv(wrap_func, 0)) {
        return SvREFCNT_inc(internal_sv);
    }

    dSP; ENTER; SAVETMPS;
    PUSHMARK(SP); XPUSHs(sv_2mortal(internal_sv)); PUTBACK;
    int count = call_pv(wrap_func, G_SCALAR);
    SPAGAIN;
    SV* wrapped = (count == 1) ? SvREFCNT_inc(POPs) : &PL_sv_undef;
    PUTBACK; FREETMPS; LEAVE;
    return wrapped;
}

SV *PerlUpb_UpbToSv(pTHX_ const upb_MessageValue *val, const upb_FieldDef *f, SV *parent_arena_sv) {
    if (!f) {
        croak("PerlUpb_UpbToSv: upb_FieldDef was NULL");
        return newSV(0); // Not reached
    }
    if (!val) {
        return newSV(0); // Return undef
    }

    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);

    if (upb_FieldDef_IsMap(f)) {
        upb_Map *map = (upb_Map*)val->map_val;
        if (!map) return newSV(0);
        SV* internal = PerlUpb_Map_New(aTHX_ map, f, parent_arena_sv, 0);

        if (reg && reg->stash_map_public) {
            return wrap_container_native(aTHX_ internal, reg->stash_map_public, "Protobuf::Internal::wrap_map");
        }
        return internal;
    }

    if (upb_FieldDef_IsRepeated(f)) {
        upb_Array *arr = (upb_Array*)val->array_val;
        if (!arr) return newSV(0);
        SV* internal = PerlUpb_Repeated_New(aTHX_ arr, f, parent_arena_sv, 0);

        if (reg && reg->stash_repeated_public) {
            return wrap_container_native(aTHX_ internal, reg->stash_repeated_public, "Protobuf::Internal::wrap_repeated");
        }
        return internal;
    } else {
        return convert_singular_upb_to_sv(aTHX_ val, f, parent_arena_sv);
    }
}

#include "upb/message/array.h"

SV *PerlUpb_Message_ToSv(pTHX_ const upb_Message *msg, const upb_MessageDef *mdef, SV *parent_arena_sv) {
    if (!msg || !mdef) return newSV(0);

    HV *hv = newHV();
    int n = upb_MessageDef_FieldCount(mdef);
    for (int i = 0; i < n; i++) {
        const upb_FieldDef *f = upb_MessageDef_Field(mdef, i);
        const char *name = upb_FieldDef_Name(f);

        bool has = false;
        if (upb_FieldDef_IsRepeated(f)) {
            upb_MessageValue v = upb_Message_GetFieldByDef(msg, f);
            has = (v.array_val != NULL && upb_Array_Size(v.array_val) > 0);
        } else if (upb_FieldDef_IsMap(f)) {
            upb_MessageValue v = upb_Message_GetFieldByDef(msg, f);
            has = (v.map_val != NULL && upb_Map_Size(v.map_val) > 0);
        } else {
            has = upb_Message_HasFieldByDef(msg, f);
        }

        if (has) {
            upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
            SV *val_sv;

            if (upb_FieldDef_IsMap(f)) {
                // ... (Map logic unchanged)
                const upb_MessageDef *entry_mdef = upb_FieldDef_MessageSubDef(f);
                const upb_FieldDef *key_f = upb_MessageDef_FindFieldByNumber(entry_mdef, 1);
                const upb_FieldDef *val_f = upb_MessageDef_FindFieldByNumber(entry_mdef, 2);

                HV *map_hv = newHV();
                upb_Map *map = (upb_Map*)val.map_val;
                size_t iter = kUpb_Map_Begin;
                upb_MessageValue k, v;
                while (upb_Map_Next(map, &k, &v, &iter)) {
                    SV *k_sv = PerlUpb_UpbToSv_Element(aTHX_ &k, key_f, parent_arena_sv);
                    SV *v_sv;
                    if (upb_FieldDef_IsSubMessage(val_f)) {
                        v_sv = PerlUpb_Message_ToSv(aTHX_ v.msg_val, upb_FieldDef_MessageSubDef(val_f), parent_arena_sv);
                    } else {
                        v_sv = PerlUpb_UpbToSv_Element(aTHX_ &v, val_f, parent_arena_sv);
                    }
                    hv_store_ent(map_hv, k_sv, v_sv, 0);
                    SvREFCNT_dec(k_sv);
                }
                val_sv = newRV_noinc((SV*)map_hv);
            }
            else if (upb_FieldDef_IsRepeated(f)) {
                upb_Array *arr = (upb_Array*)val.array_val;
                size_t size = upb_Array_Size(arr);
                AV *av = newAV();
                av_extend(av, size);

                if (!upb_FieldDef_IsSubMessage(f) && upb_FieldDef_Type(f) != kUpb_FieldType_String && upb_FieldDef_Type(f) != kUpb_FieldType_Bytes) {
                    // FAST PATH: Batch conversion for scalar arrays
                    const void *data = upb_Array_DataPtr(arr);
                    SV **entries = (SV**)safemalloc(size * sizeof(SV*));
                    PerlUpb_UpbToSv_BatchRaw(aTHX_ data, upb_FieldDef_CType(f), entries, size);
                    for (size_t j = 0; j < size; j++) {
                        av_push(av, entries[j]);
                    }
                    safefree(entries);
                } else {
                    for (size_t j = 0; j < size; j++) {
                        upb_MessageValue item = upb_Array_Get(arr, j);
                        SV *item_sv;
                        if (upb_FieldDef_IsSubMessage(f)) {
                            item_sv = PerlUpb_Message_ToSv(aTHX_ item.msg_val, upb_FieldDef_MessageSubDef(f), parent_arena_sv);
                        } else {
                            item_sv = PerlUpb_UpbToSv_Element(aTHX_ &item, f, parent_arena_sv);
                        }
                        av_push(av, item_sv);
                    }
                }
                val_sv = newRV_noinc((SV*)av);
            }
            else if (upb_FieldDef_IsSubMessage(f)) {
                val_sv = PerlUpb_Message_ToSv(aTHX_ val.msg_val, upb_FieldDef_MessageSubDef(f), parent_arena_sv);
            }
            else {
                val_sv = convert_singular_upb_to_sv(aTHX_ &val, f, parent_arena_sv);
            }

            hv_store(hv, name, strlen(name), val_sv, 0);
        }
    }

    return newRV_noinc((SV*)hv);
}

SV *PerlUpb_Message_ToPerl(pTHX_ SV *message_sv) {
    const upb_Message *msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef *mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    SV *arena_sv = PerlUpb_Message_GetArena(aTHX_ message_sv);

    if (!msg || !mdef) croak("Invalid message object for to_perl");

    return PerlUpb_Message_ToSv(aTHX_ msg, mdef, arena_sv);
}
