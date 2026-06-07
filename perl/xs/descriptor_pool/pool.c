#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_pool/pool.h"
#include "xs/protobuf/obj_cache.h"

typedef struct {
    upb_DefPool* pool;
    bool frozen;
} PerlUpb_DescriptorPool;

static upb_DefPool *generated_pool_ptr = NULL;

void* PerlUpb_DescriptorPool_CreateRaw(pTHX) {
    PerlUpb_DescriptorPool* p = (PerlUpb_DescriptorPool*)safemalloc(sizeof(PerlUpb_DescriptorPool));
    p->pool = upb_DefPool_New();
    p->frozen = false;
    if (!p->pool) {
        safefree(p);
        croak("Failed to create upb_DefPool");
    }
    return p;
}

void PerlUpb_DescriptorPool_DestroyRaw(pTHX_ void* ptr) {
    if (PL_dirty) return;
    PerlUpb_DescriptorPool* p = (PerlUpb_DescriptorPool*)ptr;
    if (p) {
        if (p->pool && p->pool != generated_pool_ptr) {
            PerlUpb_ObjCache_Delete(aTHX_ p->pool);
            upb_DefPool_Free(p->pool);
        }
        safefree(p);
    }
}

void PerlUpb_DescriptorPool_Freeze(pTHX_ SV* sv) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ sv);
    if (pool) {
        SV* rv = SvRV(sv);
        SV** svp = hv_fetch((HV*)rv, "_pool_ptr", 9, 0);
        PerlUpb_DescriptorPool* p = INT2PTR(PerlUpb_DescriptorPool*, SvIV(*svp));
        p->frozen = true;
    }
}

bool PerlUpb_DescriptorPool_IsFrozen(pTHX_ SV* sv) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ sv);
    if (pool) {
        SV* rv = SvRV(sv);
        SV** svp = hv_fetch((HV*)rv, "_pool_ptr", 9, 0);
        PerlUpb_DescriptorPool* p = INT2PTR(PerlUpb_DescriptorPool*, SvIV(*svp));
        return p->frozen;
    }
    return false;
}

SV* PerlUpb_DescriptorPool_New(pTHX) {
    PerlUpb_DescriptorPool* raw = (PerlUpb_DescriptorPool*)PerlUpb_DescriptorPool_CreateRaw(aTHX);
    HV* hv = newHV();
    hv_store(hv, "_pool_ptr", 9, newSViv(PTR2IV(raw)), 0);

    SV* obj = newRV_noinc((SV*)hv);
    sv_bless(obj, gv_stashpv("Protobuf::DescriptorPool", GV_ADD));

    PerlUpb_ObjCache_Add(aTHX_ raw->pool, obj);
    return obj;
}

SV* PerlUpb_DescriptorPool_GetWrapper(pTHX_ const upb_DefPool* pool) {
    if (!pool) return &PL_sv_undef;

    SV* cached = PerlUpb_ObjCache_Get(aTHX_ pool);
    if (cached) {
        return cached;
    }

    // If it's not cached, it's likely the generated pool or we're wrapping a pointer from C.
    // For now, we'll create a new wrapper.
    HV* hv = newHV();
    PerlUpb_DescriptorPool* p = (PerlUpb_DescriptorPool*)safemalloc(sizeof(PerlUpb_DescriptorPool));
    p->pool = (upb_DefPool*)pool;
    p->frozen = false;
    hv_store(hv, "_pool_ptr", 9, newSViv(PTR2IV(p)), 0);

    SV* obj = newRV_noinc((SV*)hv);
    sv_bless(obj, gv_stashpv("Protobuf::DescriptorPool", GV_ADD));

    PerlUpb_ObjCache_Add(aTHX_ pool, obj);
    return obj;
}

const upb_DefPool* PerlUpb_DescriptorPool_GetPool(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_isa(sv, "Protobuf::DescriptorPool")) {
        croak("Argument is not a Protobuf::DescriptorPool object");
    }

    SV* rv = SvRV(sv);
    if (SvTYPE(rv) != SVt_PVHV) return NULL;

    SV** svp = hv_fetch((HV*)rv, "_pool_ptr", 9, 0);
    if (!svp || !SvIOK(*svp)) return NULL;

    PerlUpb_DescriptorPool* p = INT2PTR(PerlUpb_DescriptorPool*, SvIV(*svp));
    return p ? p->pool : NULL;
}

void PerlUpb_DescriptorPool_Free(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_isa(sv, "Protobuf::DescriptorPool")) {
        return;
    }

    SV* rv = SvRV(sv);
    if (SvTYPE(rv) != SVt_PVHV) return;

    SV** svp = hv_fetch((HV*)rv, "_pool_ptr", 9, 0);
    if (svp && SvIOK(*svp)) {
        void* ptr = INT2PTR(void*, SvIV(*svp));
        PerlUpb_DescriptorPool_DestroyRaw(aTHX_ ptr);
        sv_setiv(*svp, 0);
    }
}

const upb_DefPool* PerlUpb_DescriptorPool_GetPoolRaw(pTHX_ void* ptr) {
    PerlUpb_DescriptorPool* p = (PerlUpb_DescriptorPool*)ptr;
    return p ? p->pool : NULL;
}

#include "xs/descriptor/file.h"

int PerlUpb_DescriptorPool_FileCount(pTHX_ SV* sv) {
    // upb doesn't easily expose the number of files in a pool without iterating.
    // For now, return 0 or implement a basic tracking if needed.
    // Actually, in our TestHelpers, we only load one set.
    return 0;
}

SV* PerlUpb_DescriptorPool_GetFile(pTHX_ SV* sv, int index) {
    return &PL_sv_undef;
}

SV* PerlUpb_DescriptorPool_GeneratedPool(pTHX) {
    if (!generated_pool_ptr) {
        SV* global_pool_sv = get_sv("Protobuf::DescriptorPool::_generated_pool_ptr", GV_ADD);
        if (SvIOK(global_pool_sv)) {
            generated_pool_ptr = INT2PTR(upb_DefPool*, SvIV(global_pool_sv));
        } else {
            generated_pool_ptr = upb_DefPool_New();
            sv_setiv(global_pool_sv, PTR2IV(generated_pool_ptr));
            // Add magic to the global SV to ensure it's not tampered with
            // and we can potentially hook its destruction.
            sv_magicext(global_pool_sv, NULL, PERL_MAGIC_ext, NULL, (const char*)NULL, 0);
        }
    }
    return PerlUpb_DescriptorPool_GetWrapper(aTHX_ generated_pool_ptr);
}
