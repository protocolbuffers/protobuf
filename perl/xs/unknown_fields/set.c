#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/unknown_fields/set.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/arena.h"
#include "upb/message/message.h"
#include "upb/message/internal/message.h"
#include "upb/wire/reader.h"
#include "upb/wire/eps_copy_input_stream.h"

// MUST be last.
#include "upb/port/def.inc"

typedef struct {
    SV* message_sv;
} PerlUpb_UnknownFieldSet;

SV* PerlUpb_UnknownFieldSet_New(pTHX_ SV* message_sv) {
    PerlUpb_UnknownFieldSet* s = (PerlUpb_UnknownFieldSet*)malloc(sizeof(PerlUpb_UnknownFieldSet));
    s->message_sv = newSVsv(message_sv);

    SV* sv = newSViv((IV)s);
    SV* obj = newRV_noinc(sv);
    sv_bless(obj, gv_stashpv("Protobuf::UnknownFieldSet", GV_ADD));
    return obj;
}

static PerlUpb_UnknownFieldSet* GetSet(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, "Protobuf::UnknownFieldSet")) {
        return NULL;
    }
    return (PerlUpb_UnknownFieldSet*)SvIV(SvRV(sv));
}

SV* PerlUpb_UnknownFieldSet_GetData(pTHX_ SV* self) {
    PerlUpb_UnknownFieldSet* s = GetSet(aTHX_ self);
    if (!s) return &PL_sv_undef;

    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ s->message_sv);
    if (!msg) return &PL_sv_undef;

    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_StringView data;

    // First pass to calculate total length
    size_t total_len = 0;
    while (upb_Message_NextUnknown(msg, &data, &iter)) {
        total_len += data.size;
    }

    if (total_len == 0) return newSVpv("", 0);

    SV* result = newSV(total_len + 1);
    char* buf = SvPVX(result);
    size_t offset = 0;

    iter = kUpb_Message_UnknownBegin;
    while (upb_Message_NextUnknown(msg, &data, &iter)) {
        memcpy(buf + offset, data.data, data.size);
        offset += data.size;
    }
    buf[total_len] = '\0';
    SvCUR_set(result, total_len);
    SvPOK_on(result);

    return result;
}

void PerlUpb_UnknownFieldSet_Add(pTHX_ SV* self, SV* data_sv) {
    PerlUpb_UnknownFieldSet* s = GetSet(aTHX_ self);
    if (!s) return;

    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ s->message_sv);
    if (!msg) return;

    STRLEN len;
    const char* data = SvPVbyte(data_sv, len);

    SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ s->message_sv);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    UPB_PRIVATE(_upb_Message_AddUnknown)(msg, data, len, arena, kUpb_AddUnknown_Copy);
}

void PerlUpb_UnknownFieldSet_Clear(pTHX_ SV* self) {
    PerlUpb_UnknownFieldSet* s = GetSet(aTHX_ self);
    if (!s) return;

    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ s->message_sv);
    if (!msg) return;

    _upb_Message_DiscardUnknown_shallow(msg);
}

static const char* decode_varint(const char* ptr, const char* end, uint64_t* val) {
    uint64_t res = 0;
    int shift = 0;
    while (ptr < end) {
        uint8_t byte = (uint8_t)*ptr++;
        res |= (uint64_t)(byte & 0x7f) << shift;
        if (!(byte & 0x80)) {
            *val = res;
            return ptr;
        }
        shift += 7;
        if (shift >= 64) break;
    }
    return NULL;
}

static const char* skip_value(const char* ptr, const char* end, int wire_type) {
    switch (wire_type) {
        case 0: { // Varint
            uint64_t dummy;
            return decode_varint(ptr, end, &dummy);
        }
        case 1: // 64-bit
            if (end - ptr < 8) return NULL;
            return ptr + 8;
        case 2: { // Length-delimited
            uint64_t len;
            ptr = decode_varint(ptr, end, &len);
            if (!ptr || (size_t)(end - ptr) < len) return NULL;
            return ptr + len;
        }
        case 5: // 32-bit
            if (end - ptr < 4) return NULL;
            return ptr + 4;
        default:
            return NULL; // Groups or unknown wire types not supported for simple scrubbing
    }
}

void PerlUpb_UnknownFieldSet_DeleteTag(pTHX_ SV* self, uint32_t tag) {
    PerlUpb_UnknownFieldSet* s = GetSet(aTHX_ self);
    if (!s) return;

    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ s->message_sv);
    if (!msg) return;

    SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ s->message_sv);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    upb_Arena* tmp_arena = upb_Arena_New();
    size_t total_size = 0;
    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_StringView chunk;

    while (upb_Message_NextUnknown(msg, &chunk, &iter)) {
        total_size += chunk.size;
    }

    char* new_data = (char*)upb_Arena_Malloc(tmp_arena, total_size);
    size_t new_size = 0;
    iter = kUpb_Message_UnknownBegin;

    while (upb_Message_NextUnknown(msg, &chunk, &iter)) {
        const char* ptr = chunk.data;
        const char* end = ptr + chunk.size;

        while (ptr < end) {
            const char* entry_start = ptr;
            uint64_t entry_tag;

            const char* tag_end = decode_varint(ptr, end, &entry_tag);
            if (!tag_end) break;

            const char* entry_end = skip_value(tag_end, end, entry_tag & 7);
            if (!entry_end) break;

            if ((entry_tag >> 3) != tag) {
                size_t entry_len = entry_end - entry_start;
                memcpy(new_data + new_size, entry_start, entry_len);
                new_size += entry_len;
            }
            ptr = entry_end;
        }
    }

    _upb_Message_DiscardUnknown_shallow(msg);
    if (new_size > 0) {
        UPB_PRIVATE(_upb_Message_AddUnknown)(msg, new_data, new_size, arena, kUpb_AddUnknown_Copy);
    }

    upb_Arena_Free(tmp_arena);
}

void PerlUpb_UnknownFieldSet_Free(pTHX_ SV* sv) {
    if (PL_dirty) return;
    PerlUpb_UnknownFieldSet* s = GetSet(aTHX_ sv);
    if (s) {
        SvREFCNT_dec(s->message_sv);
        free(s);
        sv_setiv(SvRV(sv), 0);
    }
}
