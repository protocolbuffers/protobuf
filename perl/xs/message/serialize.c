#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/message/serialize.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/arena.h"
#include "xs/descriptor/message.h"
#include "xs/descriptor_pool/pool.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/text/encode.h"
#include "upb/json/encode.h"
#include "upb/json/decode.h"
#include "upb/reflection/def.h"

SV* PerlUpb_Message_Parse(pTHX_ SV* descriptor_sv, SV* data_sv) {
    const upb_MessageDef *mdef = PerlUpb_MessageDef_GetMessage(aTHX_ descriptor_sv);
    if (!mdef) {
        croak("descriptor_sv must be a Protobuf::MessageDescriptor");
    }

    STRLEN len;
    const char* data = SvPVbyte(data_sv, len);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef);
    if (!mt) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to get MiniTable for message");
    }

    upb_Message *msg = upb_Message_New(mt, arena);
    if (!msg) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to allocate upb_Message");
    }

    // TODO: Support extension registries
    upb_DecodeStatus status = upb_Decode(data, len, msg, mt, NULL, 0, arena);
    if (status != kUpb_DecodeStatus_Ok) {
        SvREFCNT_dec(arena_sv);
        PerlUpb_DecodeStatus_Die(aTHX_ status, "parse message");
    }

    SV *msg_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
    SvREFCNT_dec(arena_sv);

    return msg_sv;
}

void PerlUpb_Message_ParseFrom(pTHX_ SV* message_sv, SV* data_sv) {
    upb_Message* msg = (upb_Message*)PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    STRLEN len;
    const char* data = SvPVbyte(data_sv, len);

    SV* arena_sv = PerlUpb_Message_GetArena(aTHX_ message_sv);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);

    // upb_Decode merges into the existing message
    upb_DecodeStatus status = upb_Decode(data, len, msg, mt, NULL, 0, arena);
    if (status != kUpb_DecodeStatus_Ok) {
        PerlUpb_DecodeStatus_Die(aTHX_ status, "parse and merge message");
    }
}

SV* PerlUpb_Message_Serialize(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) {
        croak("Invalid message object");
    }

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef);
    if (!mt) croak("Failed to get MiniTable");

    upb_Arena* enc_arena = PerlUpb_Arena_Acquire(aTHX_ PERL_UPB_LIFECYCLE_TRANSIENT);
    char* buf = NULL;
    size_t size = 0;

    upb_EncodeStatus status = upb_Encode(msg, mt, kUpb_EncodeOption_CheckRequired, enc_arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
        PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
        if (status == kUpb_EncodeStatus_MissingRequired) {
            croak("Failed to serialize message: Missing required fields");
        }
        croak("Failed to serialize message: %d", status);
    }

    SV* result = newSVpvn(buf, size);
    PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
    return result;
}

SV* PerlUpb_Message_Serialize_Deterministic(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) {
        croak("Invalid message object");
    }

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef);
    if (!mt) croak("Failed to get MiniTable");

    upb_Arena* enc_arena = PerlUpb_Arena_Acquire(aTHX_ PERL_UPB_LIFECYCLE_TRANSIENT);
    char* buf = NULL;
    size_t size = 0;

    upb_EncodeStatus status = upb_Encode(msg, mt, kUpb_EncodeOption_Deterministic | kUpb_EncodeOption_CheckRequired, enc_arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
        PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
        if (status == kUpb_EncodeStatus_MissingRequired) {
            croak("Failed to serialize message deterministically: Missing required fields");
        }
        croak("Failed to serialize message deterministically: %d", status);
    }

    SV* result = newSVpvn(buf, size);
    PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
    return result;
}

SV* PerlUpb_Message_ToText(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) {
        croak("Invalid message object");
    }

    size_t size = upb_TextEncode(msg, mdef, NULL, 0, NULL, 0);

    char* buf = (char*)malloc(size + 1);
    if (!buf) croak("Out of memory encoding text format");

    size_t encoded = upb_TextEncode(msg, mdef, NULL, 0, buf, size + 1);

    SV* result = newSVpvn(buf, encoded);
    SvUTF8_on(result);
    free(buf);
    return result;
}

SV* PerlUpb_Message_ToJson(pTHX_ SV* message_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    const upb_DefPool* ext_pool = upb_FileDef_Pool(upb_MessageDef_File(mdef));
    upb_Status status;
    upb_Status_Clear(&status);

    size_t size = upb_JsonEncode(msg, mdef, ext_pool, 0, NULL, 0, &status);
    if (!upb_Status_IsOk(&status)) {
        croak("JSON Encode error: %s", upb_Status_ErrorMessage(&status));
    }

    char* buf = (char*)malloc(size + 1);
    if (!buf) croak("Out of memory encoding JSON");

    size_t encoded = upb_JsonEncode(msg, mdef, ext_pool, 0, buf, size + 1, &status);
    if (!upb_Status_IsOk(&status)) {
        free(buf);
        croak("JSON Encode error: %s", upb_Status_ErrorMessage(&status));
    }

    SV* result = newSVpvn(buf, encoded);
    SvUTF8_on(result);
    free(buf);
    return result;
}

void PerlUpb_Message_JsonToHandle(pTHX_ SV* message_sv, SV* fh_sv) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    IO* io = sv_2io(fh_sv);
    if (!io) croak("Invalid file handle");
    PerlIO* fp = IoOFP(io);
    if (!fp) croak("File handle not open for writing");

    const upb_DefPool* ext_pool = upb_FileDef_Pool(upb_MessageDef_File(mdef));
    upb_Status status;
    upb_Status_Clear(&status);

    size_t size = upb_JsonEncode(msg, mdef, ext_pool, 0, NULL, 0, &status);
    if (!upb_Status_IsOk(&status)) {
        croak("JSON Encode error: %s", upb_Status_ErrorMessage(&status));
    }

    char* buf = (char*)malloc(size + 1);
    if (!buf) croak("Out of memory encoding JSON");

    size_t encoded = upb_JsonEncode(msg, mdef, ext_pool, 0, buf, size + 1, &status);
    if (!upb_Status_IsOk(&status)) {
        free(buf);
        croak("JSON Encode error: %s", upb_Status_ErrorMessage(&status));
    }

    PerlIO_write(fp, buf, encoded);
    free(buf);
}

SV* PerlUpb_Message_FromJson(pTHX_ SV* descriptor_sv, SV* json_sv, SV* options_sv) {
    const upb_MessageDef* mdef = PerlUpb_MessageDef_GetMessage(aTHX_ descriptor_sv);
    if (!mdef) {
        croak("descriptor_sv must be a Protobuf::MessageDescriptor");
    }

    // Parse options
    int options = 0;
    if (options_sv && SvOK(options_sv) && SvROK(options_sv) && SvTYPE(SvRV(options_sv)) == SVt_PVHV) {
        HV* hv = (HV*)SvRV(options_sv);
        SV** ignore_unknown_sv = hv_fetch(hv, "ignore_unknown", 14, 0);
        if (ignore_unknown_sv && SvTRUE(*ignore_unknown_sv)) {
            options |= upb_JsonDecode_IgnoreUnknown;
        }
    }

    STRLEN len;
    const char* json_str = SvPVutf8(json_sv, len);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    const upb_MiniTable *mt = upb_MessageDef_MiniTable(mdef);
    if (!mt) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to get MiniTable for message");
    }

    upb_Message *msg = upb_Message_New(mt, arena);
    if (!msg) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to allocate upb_Message");
    }

    const upb_DefPool* ext_pool = upb_FileDef_Pool(upb_MessageDef_File(mdef));
    upb_Status status;
    upb_Status_Clear(&status);

    bool ok = upb_JsonDecode(json_str, len, msg, mdef, ext_pool, options, arena, &status);
    if (!ok) {
        SvREFCNT_dec(arena_sv);
        croak("Failed to parse JSON: %s", upb_Status_ErrorMessage(&status));
    }

    SV *msg_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
    SvREFCNT_dec(arena_sv);

    return msg_sv;
}

static void WriteVarint(pTHX_ PerlIO* fp, uint64_t val) {
    uint8_t buf[10];
    int i = 0;
    do {
        buf[i] = (uint8_t)(val & 0x7F);
        val >>= 7;
        if (val > 0) buf[i] |= 0x80;
        i++;
    } while (val > 0);
    PerlIO_write(fp, buf, i);
}

void PerlUpb_Message_ToHandle(pTHX_ SV* message_sv, SV* fh_sv, bool length_prefixed) {
    const upb_Message* msg = PerlUpb_Message_GetMsg(aTHX_ message_sv);
    const upb_MessageDef* mdef = PerlUpb_Message_GetDef(aTHX_ message_sv);
    if (!msg || !mdef) croak("Invalid message object");

    IO* io = sv_2io(fh_sv);
    if (!io) croak("Invalid file handle");
    PerlIO* fp = IoOFP(io);
    if (!fp) croak("File handle not open for writing");

    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);
    upb_Arena* enc_arena = PerlUpb_Arena_Acquire(aTHX_ PERL_UPB_LIFECYCLE_TRANSIENT);
    char* buf = NULL;
    size_t size = 0;

    upb_EncodeStatus status = upb_Encode(msg, mt, kUpb_EncodeOption_CheckRequired, enc_arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
        PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
        croak("Serialization failed: %d", status);
    }

    if (length_prefixed) {
        WriteVarint(aTHX_ fp, size);
    }

    if (size > 0) {
        SSize_t written = PerlIO_write(fp, buf, size);
        if (written < (SSize_t)size) {
            PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
            croak("Failed to write full message to handle: %d of %zu bytes written", (int)written, size);
        }
    }

    PerlUpb_Arena_Release(aTHX_ enc_arena, PERL_UPB_LIFECYCLE_TRANSIENT);
}

static uint64_t ReadVarint(pTHX_ PerlIO* fp, bool* ok) {
    uint64_t val = 0;
    int shift = 0;
    uint8_t byte;
    *ok = false;

    while (shift < 64) {
        if (PerlIO_read(fp, &byte, 1) != 1) return 0;
        val |= (uint64_t)(byte & 0x7F) << shift;
        if (!(byte & 0x80)) {
            *ok = true;
            return val;
        }
        shift += 7;
    }
    return 0;
}

SV* PerlUpb_Message_FromHandle(pTHX_ SV* descriptor_sv, SV* fh_sv, bool length_prefixed) {
    const upb_MessageDef* mdef = PerlUpb_MessageDef_GetMessage(aTHX_ descriptor_sv);
    if (!mdef) croak("descriptor_sv must be a Protobuf::MessageDescriptor");

    IO* io = sv_2io(fh_sv);
    if (!io) croak("Invalid file handle");
    PerlIO* fp = IoIFP(io);
    if (!fp) croak("File handle not open for reading");

    SV* arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    char* buf = NULL;
    size_t total_to_read = 0;

    if (length_prefixed) {
        bool ok;
        total_to_read = (size_t)ReadVarint(aTHX_ fp, &ok);
        if (!ok) {
            SvREFCNT_dec(arena_sv);
            return &PL_sv_undef; // EOF or error reading length
        }
        buf = upb_Arena_Malloc(arena, total_to_read);
        SSize_t n = PerlIO_read(fp, buf, total_to_read);
        if (n < (SSize_t)total_to_read) {
            SvREFCNT_dec(arena_sv);
            croak("Unexpected EOF reading length-prefixed message: expected %zu, got %zd", total_to_read, n);
        }
    } else {
        // Read until EOF
        size_t capacity = 4096;
        size_t total_read = 0;
        buf = upb_Arena_Malloc(arena, capacity);

        while (1) {
            if (total_read == capacity) {
                size_t new_capacity = capacity * 2;
                char* new_buf = upb_Arena_Malloc(arena, new_capacity);
                memcpy(new_buf, buf, total_read);
                buf = new_buf;
                capacity = new_capacity;
            }

            SSize_t n = PerlIO_read(fp, buf + total_read, capacity - total_read);
            if (n <= 0) break;
            total_read += n;
        }
        total_to_read = total_read;
    }

    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);
    upb_Message* msg = upb_Message_New(mt, arena);

    upb_DecodeStatus status = upb_Decode(buf, total_to_read, msg, mt, NULL, 0, arena);
    if (status != kUpb_DecodeStatus_Ok) {
        SvREFCNT_dec(arena_sv);
        PerlUpb_DecodeStatus_Die(aTHX_ status, "parse message from handle");
    }

    SV* msg_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
    SvREFCNT_dec(arena_sv);
    return msg_sv;
}
