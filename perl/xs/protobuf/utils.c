#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/protobuf/utils.h"
#include "xs/protobuf/registry.h"
#include "xs/protobuf/obj_cache.h"
#include "upb/reflection/def.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#include <cpuid.h>
#define HAS_X86_INTRINSICS
#endif

#define AVX2_INSTRUMENT(path_name) \
    do { \
        if (getenv("PROTOBUF_PERL_INSTRUMENT_AVX2")) { \
            fprintf(stderr, "[AVX2] Hitting path: %s\n", path_name); \
        } \
    } while (0)

const char* PerlUpb_GetStrData(pTHX_ SV *sv) {
    if (!sv || !SvPOK(sv)) {
        return NULL;
    }
    STRLEN len;
    return SvPVutf8(sv, len);
}

const char* PerlUpb_VerifyStrData(pTHX_ SV *sv) {
    if (!sv || !SvPOK(sv)) {
        croak("Expected a string SV");
    }
    STRLEN len;
    return SvPVutf8(sv, len);
}

#ifdef HAS_X86_INTRINSICS
// Helper to check for dots or colons in 32-byte chunks
__attribute__((target("avx2")))
static inline uint32_t find_special_chars_avx2(const char* s) {
    AVX2_INSTRUMENT("find_special_chars_avx2");
    __m256i chunk = _mm256_loadu_si256((const __m256i*)s);
    __m256i dots = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('.'));
    __m256i colons = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8(':'));
    __m256i special = _mm256_or_si256(dots, colons);
    return (uint32_t)_mm256_movemask_epi8(special);
}
#endif

#ifdef HAS_X86_INTRINSICS
__attribute__((target("avx2")))
#endif
char* PerlUpb_ClassNameToFullName(pTHX_ const char* class_name) {
    if (!class_name) return NULL;
    STRLEN len = strlen(class_name);
    char* full_name = (char*)safemalloc(len + 1);
    char* d = full_name;
    const char* s = class_name;

#ifdef HAS_X86_INTRINSICS
    STRLEN remaining = len;
    // AVX2 Optimization for 32-byte chunks
    while (remaining >= 32) {
        uint32_t mask = find_special_chars_avx2(s);
        if (mask == 0) {
            _mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((const __m256i*)s));
            d += 32;
            s += 32;
            remaining -= 32;
        } else {
            break;
        }
    }

    // SSE4.1 Fallback for 16-byte chunks
    while (remaining >= 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)s);
        __m128i dots = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('.'));
        __m128i colons = _mm_cmpeq_epi8(chunk, _mm_set1_epi8(':'));
        uint32_t mask = (uint32_t)_mm_movemask_epi8(_mm_or_si128(dots, colons));
        if (mask == 0) {
            _mm_storeu_si128((__m128i*)d, chunk);
            d += 16;
            s += 16;
            remaining -= 16;
        } else {
            break;
        }
    }
#endif

    while (*s) {
        if (*s == ':' && *(s+1) == ':') {
            *d++ = '.';
            s += 2;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
    return full_name;
}

#ifdef HAS_X86_INTRINSICS
__attribute__((target("avx2")))
#endif
char* PerlUpb_FullNameToClassName(pTHX_ const char* full_name) {
    if (!full_name) return NULL;
    STRLEN len = strlen(full_name);
    int dots = 0;
    for (const char* p = full_name; *p; p++) if (*p == '.') dots++;

    char* class_name = (char*)safemalloc(len + dots + 1);
    char* d = class_name;
    const char* s = full_name;

#ifdef HAS_X86_INTRINSICS
    STRLEN remaining = len;
    // AVX2 Optimization for 32-byte chunks (no dots)
    while (remaining >= 32) {
        uint32_t mask = find_special_chars_avx2(s);
        if (mask == 0) {
            _mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((const __m256i*)s));
            d += 32;
            s += 32;
            remaining -= 32;
        } else {
            break;
        }
    }
#endif

    while (*s) {
        if (*s == '.') {
            *d++ = ':';
            *d++ = ':';
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
    return class_name;
}

static char* capitalize_path(pTHX_ const char* s) {
    if (!s || !*s) return NULL;
    char* res = (char*)safemalloc(strlen(s) * 2 + 1);
    char* d = res;
    bool first = true;
    while (*s) {
        if (first || *(s-1) == '.') {
            if (*s >= 'a' && *s <= 'z') *d++ = *s - 32;
            else *d++ = *s;
        } else if (*s == '.') {
            *d++ = ':'; *d++ = ':';
        } else {
            *d++ = *s;
        }
        first = false;
        s++;
    }
    *d = '\0';
    return res;
}

static char* get_file_module(pTHX_ const char* proto_file) {
    const char* last_slash = strrchr(proto_file, '/');
    const char* start = last_slash ? last_slash + 1 : proto_file;
    char* base = savepv(start);
    char* dot = strchr(base, '.');
    if (dot) *dot = '\0';

    // CamelCase by splitting on _
    char* res = (char*)safemalloc(strlen(base) + 1);
    char* d = res;
    bool first = true;
    for (char* s = base; *s; s++) {
        if (first || *(s-1) == '_') {
            if (*s == '_') continue;
            if (*s >= 'a' && *s <= 'z') *d++ = *s - 32;
            else *d++ = *s;
        } else if (*s == '_') {
            // skip
        } else {
            *d++ = *s;
        }
        first = false;
    }
    *d = '\0';
    safefree(base);
    return res;
}

HV* PerlUpb_GetMessageStash(pTHX_ const upb_MessageDef* mdef) {
    if (!mdef) return gv_stashpv("Protobuf::Message", GV_ADD);

    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (reg && reg->stash_cache) {
        SV** svp = hv_fetch(reg->stash_cache, (const char*)&mdef, sizeof(mdef), 0);
        if (svp && SvIOK(*svp)) {
            return INT2PTR(HV*, SvIV(*svp));
        }
    }

    const char *full_name = upb_MessageDef_FullName(mdef);
    const upb_FileDef *file = upb_MessageDef_File(mdef);
    const char *proto_file = upb_FileDef_Name(file);
    const char *pkg = upb_FileDef_Package(file);

    char* file_mod = get_file_module(aTHX_ proto_file);
    char* pkg_mod = capitalize_path(aTHX_ pkg);

    const char* rel_name = full_name;
    if (rel_name[0] == '.') rel_name++;
    if (pkg && strlen(pkg) > 0) {
        if (strncmp(rel_name, pkg, strlen(pkg)) == 0) {
            rel_name += strlen(pkg);
            if (rel_name[0] == '.') rel_name++;
        }
    }
    char* msg_path = capitalize_path(aTHX_ rel_name);

    if (!file_mod) file_mod = savepv("UnknownFile");
    if (!msg_path) msg_path = savepv("UnknownMessage");

    size_t total_len = (pkg_mod ? strlen(pkg_mod) + 2 : 0) + strlen(file_mod) + 2 + strlen(msg_path) + 1;
    char* class_name = (char*)safemalloc(total_len);
    if (pkg_mod) {
        sprintf(class_name, "%s::%s::%s", pkg_mod, file_mod, msg_path);
        safefree(pkg_mod);
    } else {
        sprintf(class_name, "%s::%s", file_mod, msg_path);
    }
    safefree(file_mod);
    safefree(msg_path);

    HV* stash = gv_stashpv(class_name, GV_ADD);

    // Set up inheritance: push "Protobuf::Message" to @ISA if not already present
    {
        char isa_name[1024];
        snprintf(isa_name, sizeof(isa_name), "%s::ISA", class_name);
        AV* isa = get_av(isa_name, GV_ADD);
        bool found = false;
        for (int i = 0; i <= av_len(isa); i++) {
            SV** svp = av_fetch(isa, i, 0);
            if (svp && SvPOK(*svp) && strEQ(SvPV_nolen(*svp), "Protobuf::Message")) {
                found = true;
                break;
            }
        }
        if (!found) {
            av_push(isa, newSVpv("Protobuf::Message", 0));
        }
    }

    safefree(class_name);

    if (reg && reg->stash_cache) {
        hv_store(reg->stash_cache, (const char*)&mdef, sizeof(mdef), newSViv(PTR2IV(stash)), 0);
    }

    return stash;
}

void PerlUpb_Error_Die(pTHX_ const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vcroak(fmt, &args);
    va_end(args);
}

void PerlUpb_DecodeStatus_Die(pTHX_ upb_DecodeStatus status, const char* context) {
    const char* status_str = upb_DecodeStatus_String(status);
    const char* description = "Unknown error";

    switch (status) {
        case kUpb_DecodeStatus_Ok:
            return; // Should not happen if this function is called
        case kUpb_DecodeStatus_Malformed:
            description = "Protobuf wire format is malformed or corrupt";
            break;
        case kUpb_DecodeStatus_OutOfMemory:
            description = "Out of memory during decoding (Arena allocation failed)";
            break;
        case kUpb_DecodeStatus_BadUtf8:
            description = "String field contains invalid UTF-8 data";
            break;
        case kUpb_DecodeStatus_MaxDepthExceeded:
            description = "Message nesting depth exceeds the configured limit";
            break;
        case kUpb_DecodeStatus_MissingRequired:
            description = "Message is missing required fields (proto2 only)";
            break;
    }

    if (context) {
        croak("Failed to %s: %s (%s)", context, description, status_str);
    } else {
        croak("Protobuf decode error: %s (%s)", description, status_str);
    }
}

static int wrapper_cleanup(pTHX_ SV* sv, MAGIC* mg) {
    if (PL_dirty) return 0;
    void* ptr = (void*)mg->mg_ptr;
    if (ptr) {
        PerlUpb_ObjCache_Delete(aTHX_ ptr);
    }
    return 0;
}

static MGVTBL wrapper_vtbl = {
    NULL, NULL, NULL, NULL, wrapper_cleanup
};

MAGIC* PerlUpb_GetMagic(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv)) return NULL;
    return mg_findext(SvRV(sv), PERL_MAGIC_ext, &wrapper_vtbl);
}

SV* PerlUpb_WrapArenaBoundObject(pTHX_ const void* ptr, SV* arena_sv, HV* stash, uint16_t flags) {
    if (!ptr) return &PL_sv_undef;

    SV* cached = PerlUpb_ObjCache_Get(aTHX_ ptr);
    if (cached) return cached;

    HV* hv = newHV();

    SV* ptr_sv = newSViv(PTR2IV(ptr));
    hv_store(hv, "_upb_ptr", 8, ptr_sv, 0);
    if (arena_sv && SvOK(arena_sv)) {
        hv_store(hv, "_arena_sv", 9, newSVsv(arena_sv), 0);
    }

    SV* self = newRV_noinc((SV*)hv);
    sv_bless(self, stash);

    // Add magic for cache cleanup. We don't use MGf_COPY because
    // we only want the primary owner to handle detachment.
    MAGIC* mg = sv_magicext((SV*)hv, NULL, PERL_MAGIC_ext, &wrapper_vtbl, (const char*)ptr, 0);
    if (mg) {
        mg->mg_private = flags;
    }

    PerlUpb_ObjCache_Add(aTHX_ ptr, self);
    return self;
}

const void* PerlUpb_GetArenaBoundObject(pTHX_ SV* sv, const char* class_name) {
    if (!sv || !SvOK(sv)) return NULL;
    void* ptr = (void*)PerlUpb_GetArenaBoundObject_Silent(aTHX_ sv, class_name);
    if (!ptr) {
        if (!SvROK(sv)) {
            croak("Invalid message object: SV is not a reference (expected %s, type %d)", class_name, (int)SvTYPE(sv));
        }
        if (!sv_derived_from(sv, class_name)) {
            const char* actual_class = (SvOBJECT(SvRV(sv))) ? HvNAME(SvSTASH(SvRV(sv))) : "unblessed-ref";
            croak("Invalid message object: expected %s, got %s", class_name, actual_class);
        }
        HV* hv = (HV*)SvRV(sv);
        if (SvTYPE(hv) != SVt_PVHV) {
            croak("Invalid message object: underlying SV is type %d, not HASH", (int)SvTYPE(hv));
        }
        croak("Invalid message object: _upb_ptr missing (expected %s)", class_name);
    }
    return ptr;
}

const void* PerlUpb_GetArenaBoundObject_Silent(pTHX_ SV* sv, const char* class_name) {
    if (!sv || !SvROK(sv) || !sv_derived_from(sv, class_name)) {
        return NULL;
    }
    HV* hv = (HV*)SvRV(sv);
    if (SvTYPE(hv) != SVt_PVHV) return NULL;
    SV** svp = hv_fetch(hv, "_upb_ptr", 8, 0);
    return svp ? (const void*)SvIV(*svp) : NULL;
}

bool PerlUpb_IsXSBacked(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv)) return false;
    HV* hv = (HV*)SvRV(sv);
    if (SvTYPE(hv) != SVt_PVHV) return false;
    return hv_exists(hv, "_upb_ptr", 8);
}

SV* PerlUpb_GetArenaFromObject(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv)) return NULL;
    HV* hv = (HV*)SvRV(sv);
    if (SvTYPE(hv) != SVt_PVHV) return NULL;
    SV** svp = hv_fetch(hv, "_arena_sv", 9, 0);
    return svp ? *svp : NULL;
}

int64_t PerlUpb_SVToI64(pTHX_ SV* sv) {
    if (!sv) return 0;
    if (SvIOK(sv)) return (int64_t)SvIV(sv);
    if (SvROK(sv) && sv_derived_from(sv, "Math::BigInt")) {
        dSP; ENTER; SAVETMPS;
        PUSHMARK(SP); XPUSHs(sv); PUTBACK;
        call_method("bstr", G_SCALAR);
        SPAGAIN; SV* bstr_sv = POPs;
        const char* s = SvPV_nolen(bstr_sv);
        int64_t val = strtoll(s, NULL, 10);
        PUTBACK; FREETMPS; LEAVE;
        return val;
    }
    if (SvPOK(sv)) return (int64_t)strtoll(SvPV_nolen(sv), NULL, 10);
    return (int64_t)SvNV(sv);
}

uint64_t PerlUpb_SVToU64(pTHX_ SV* sv) {
    if (!sv) return 0;
    if (SvUOK(sv) || SvIOK(sv)) return (uint64_t)SvUV(sv);
    if (SvROK(sv) && sv_derived_from(sv, "Math::BigInt")) {
        dSP; ENTER; SAVETMPS;
        PUSHMARK(SP); XPUSHs(sv); PUTBACK;
        call_method("bstr", G_SCALAR);
        SPAGAIN; SV* bstr_sv = POPs;
        const char* s = SvPV_nolen(bstr_sv);
        uint64_t val = (uint64_t)strtoull(s, NULL, 10);
        PUTBACK; FREETMPS; LEAVE;
        return val;
    }
    if (SvPOK(sv)) return (uint64_t)strtoull(SvPV_nolen(sv), NULL, 10);
    return (uint64_t)SvNV(sv);
}

SV* PerlUpb_I64ToSV(pTHX_ int64_t val) {
    if (val >= IV_MIN && val <= IV_MAX) return newSViv((IV)val);

    // Fallback to Math::BigInt for large values
    char buf[32];
    sprintf(buf, "%" PRId64, val);

    dSP; ENTER; SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(newSVpv("Math::BigInt", 0)));
    XPUSHs(sv_2mortal(newSVpv(buf, 0)));
    PUTBACK;
    call_method("new", G_SCALAR);
    SPAGAIN; SV* bigint_sv = newSVsv(POPs);
    PUTBACK; FREETMPS; LEAVE;
    return bigint_sv;
}

static uint32_t cpu_features = 0;

void PerlUpb_InitCpuFeatures(void) {
#ifdef HAS_X86_INTRINSICS
    uint32_t eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 19)) cpu_features |= PERL_UPB_HAS_SSE41;
    }
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        if (ebx & (1 << 5)) cpu_features |= PERL_UPB_HAS_AVX2;
    }
#endif
}

uint32_t PerlUpb_GetCpuFeatures(void) {
    return cpu_features;
}

void PerlUpb_DetectCpuFeatures(void) {
    // TODO: Implement actual CPU feature detection logic here
    // For now, set to 0
    cpu_features = 0;
    if (getenv("PROTOBUF_PERL_DUMP_CPU_FEATURES")) {
      fprintf(stderr, "[CPUID] Detected features: 0x%x\n", cpu_features);
    }
}

#ifdef HAS_X86_INTRINSICS
__attribute__((target("sse4.1")))
#endif
bool PerlUpb_ValidateIntRange_SSE41(const int32_t* vals, size_t count, int32_t min, int32_t max) {
#ifdef HAS_X86_INTRINSICS
    __m128i vmin = _mm_set1_epi32(min);
    __m128i vmax = _mm_set1_epi32(max);
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128i v = _mm_loadu_si128((const __m128i*)&vals[i]);
        // v < min  =>  min > v
        // v > max
        __m128i mask = _mm_or_si128(_mm_cmpgt_epi32(vmin, v), _mm_cmpgt_epi32(v, vmax));
        if (_mm_movemask_epi8(mask) != 0) return false;
    }
    for (; i < count; i++) {
        if (vals[i] < min || vals[i] > max) return false;
    }
#else
    for (size_t i = 0; i < count; i++) {
        if (vals[i] < min || vals[i] > max) return false;
    }
#endif
    return true;
}

#ifdef HAS_X86_INTRINSICS
__attribute__((target("avx2")))
#endif
bool PerlUpb_ValidateStrings_AVX2(const char** strings, const size_t* lens, size_t count) {
    AVX2_INSTRUMENT("PerlUpb_ValidateStrings_AVX2");
    for (size_t i = 0; i < count; i++) {
        if (!strings[i] || lens[i] == 0) return false;
    }
    return true;
}

void PerlUpb_VerifyBinaryDiff(pTHX_ const char* a, size_t a_len, const char* b, size_t b_len, const char* name) {
    if (a_len == b_len && memcmp(a, b, a_len) == 0) {
        return;
    }

    fprintf(stderr, "Binary diff failure: %s\n", name);
    fprintf(stderr, "A (len %zu): ", a_len);
    for (size_t i = 0; i < a_len; i++) fprintf(stderr, "%02x", (unsigned char)a[i]);
    fprintf(stderr, "\nB (len %zu): ", b_len);
    for (size_t i = 0; i < b_len; i++) fprintf(stderr, "%02x", (unsigned char)b[i]);
    fprintf(stderr, "\n");

    croak("Binary diff verification failed: %s", name);
}

PerlUpb_FieldVector* PerlUpb_FieldVector_New(pTHX_ size_t capacity) {
    PerlUpb_FieldVector* v = (PerlUpb_FieldVector*)safemalloc(sizeof(PerlUpb_FieldVector));
    v->count = 0;
    v->capacity = capacity;
    v->fields = (const upb_FieldDef**)safemalloc(sizeof(upb_FieldDef*) * capacity);
    v->values = (SV**)safemalloc(sizeof(SV*) * capacity);
    return v;
}

void PerlUpb_FieldVector_Add(pTHX_ PerlUpb_FieldVector* v, const upb_FieldDef* f, SV* val) {
    if (v->count >= v->capacity) {
        v->capacity *= 2;
        v->fields = (const upb_FieldDef**)saferealloc((void*)v->fields, sizeof(upb_FieldDef*) * v->capacity);
        v->values = (SV**)saferealloc((void*)v->values, sizeof(SV*) * v->capacity);
    }
    v->fields[v->count] = f;
    v->values[v->count] = val; // We don't SvREFCNT_inc here, caller owns life
    v->count++;
}

void PerlUpb_FieldVector_Free(pTHX_ PerlUpb_FieldVector* v) {
    if (v) {
        safefree((void*)v->fields);
        safefree((void*)v->values);
        safefree(v);
    }
}

void PerlUpb_CroakWithContext(pTHX_ const char* msg, const upb_MessageDef* mdef,
                             const upb_FieldDef* fdef) {
    if (!mdef) {
        croak("%s", msg);
    }

    const char* mname = upb_MessageDef_FullName(mdef);
    if (!fdef) {
        croak("%s (in message %s)", msg, mname);
    }

    const char* fname = upb_FieldDef_Name(fdef);
    croak("%s (at %s.%s)", msg, mname, fname);
}

SV* PerlUpb_U64ToSV(pTHX_ uint64_t val) {
    if (val <= UV_MAX) return newSVuv((UV)val);

    // Fallback to Math::BigInt for large values
    char buf[32];
    sprintf(buf, "%" PRIu64, val);

    dSP; ENTER; SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(newSVpv("Math::BigInt", 0)));
    XPUSHs(sv_2mortal(newSVpv(buf, 0)));
    PUTBACK;
    call_method("new", G_SCALAR);
    SPAGAIN; SV* bigint_sv = newSVsv(POPs);
    PUTBACK; FREETMPS; LEAVE;
    return bigint_sv;
}
