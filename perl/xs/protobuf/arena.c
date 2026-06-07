#define PERL_NO_GET_CONTEXT
#include "xs/protobuf/arena.h"
#include "xs/protobuf.h"
#include "xs/protobuf/registry.h"
#include "xs/protobuf/obj_cache.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// -- Allocator Implementation --

static void* stats_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize, size_t size, size_t* actual_size) {
    PerlUpb_StatsAlloc* s = (PerlUpb_StatsAlloc*)alloc;

    if (s->poisoned && size > 0) {
        return NULL;
    }

    if (size == 0) {
        if (ptr) {
            PerlUpb_ObjCache_LogEvent(ALLOC_EVENT_FREE, ptr);
            free(ptr);
        }
        return NULL;
    }

    if (s->fail_probability > 0 && ((double)rand() / RAND_MAX) < s->fail_probability) {
        return NULL;
    }

    void* ret = realloc(ptr, size);
    if (ret) {
        if (oldsize == 0) {
            PerlUpb_ObjCache_LogEvent(ALLOC_EVENT_MALLOC, ret);
            s->total_reserved += size;
            s->total_blocks++;
            if (s->total_reserved > s->historical_max_size) {
                s->historical_max_size = s->total_reserved;
            }
        } else if (size > oldsize) {
            PerlUpb_ObjCache_LogEvent(ALLOC_EVENT_REALLOC, ret);
            s->total_reserved += (size - oldsize);
            if (s->total_reserved > s->historical_max_size) {
                s->historical_max_size = s->total_reserved;
            }
        } else {
            s->total_reserved -= (oldsize - size);
        }

        if (actual_size) {
            *actual_size = size;
        }
    }
    return ret;
}

upb_Arena* PerlUpb_Arena_Acquire(pTHX_ PerlUpb_ArenaLifecycle lifecycle) {
    if (lifecycle == PERL_UPB_LIFECYCLE_TRANSIENT) {
        PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
        if (reg && reg->cached_transient_arena) {
            upb_Arena* arena = reg->cached_transient_arena;
            reg->cached_transient_arena = NULL;
            return arena;
        }
    }
    return upb_Arena_New();
}

void PerlUpb_Arena_Release(pTHX_ upb_Arena* arena, PerlUpb_ArenaLifecycle lifecycle) {
    if (!arena) return;
    if (lifecycle == PERL_UPB_LIFECYCLE_TRANSIENT) {
        PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
        if (reg && !reg->cached_transient_arena) {
            upb_Arena_Free(arena);
            return;
        }
    }
    upb_Arena_Free(arena);
}

static upb_Arena* PerlUpb_Arena_AcquireWithStats(pTHX_ PerlUpb_StatsAlloc* stats_alloc) {
    memset(stats_alloc, 0, sizeof(PerlUpb_StatsAlloc));
    stats_alloc->base.func = stats_alloc_func;
    return upb_Arena_Init(NULL, 0, &stats_alloc->base);
}

// -- Arena Wrapper Functions --

void* PerlUpb_Arena_CreateRaw(pTHX) {
    PerlUpb_Arena *arena_wrapper = (PerlUpb_Arena *)safemalloc(sizeof(PerlUpb_Arena));
    if (!arena_wrapper) {
        croak("Failed to allocate PerlUpb_Arena");
    }
    arena_wrapper->arena = PerlUpb_Arena_AcquireWithStats(aTHX_ &arena_wrapper->stats_alloc);
    if (!arena_wrapper->arena) {
        safefree(arena_wrapper);
        croak("Failed to acquire upb_Arena");
    }
    return (void*)arena_wrapper;
}

void* PerlUpb_Arena_CreateRawFromUpb(pTHX_ upb_Arena* arena) {
    PerlUpb_Arena *arena_wrapper = (PerlUpb_Arena *)safemalloc(sizeof(PerlUpb_Arena));
    if (!arena_wrapper) {
        croak("Failed to allocate PerlUpb_Arena");
    }
    memset(arena_wrapper, 0, sizeof(PerlUpb_Arena));
    arena_wrapper->arena = arena;
    return (void*)arena_wrapper;
}

upb_Arena* PerlUpb_Arena_GetRaw(pTHX_ void* ptr) {
    PerlUpb_Arena *arena_wrapper = (PerlUpb_Arena *)ptr;
    return arena_wrapper ? arena_wrapper->arena : NULL;
}

void PerlUpb_Arena_DestroyRaw(pTHX_ void* ptr) {
    PerlUpb_Arena *arena_wrapper = (PerlUpb_Arena *)ptr;
    if (arena_wrapper) {
        if (arena_wrapper->arena) {
            upb_Arena_Free(arena_wrapper->arena);
            arena_wrapper->arena = NULL;
        }
        safefree(arena_wrapper);
    }
}

static int arena_cleanup(pTHX_ SV* sv, MAGIC* mg) {
    if (PL_dirty) return 0;
    void* ptr = (void*)mg->mg_ptr;
    if (ptr) {
        // Clear magic pointer immediately to prevent double-free
        mg->mg_ptr = NULL;

        bool tmpfs = false;
        SV* rv = SvRV(sv);
        if (rv && SvTYPE(rv) == SVt_PVHV) {
            SV** is_tmpfs = hv_fetch((HV*)rv, "_is_tmpfs", 9, 0);
            tmpfs = is_tmpfs && SvTRUE(*is_tmpfs);
        }
        PerlUpb_Arena_DestroyRaw_Tmpfs(aTHX_ ptr, tmpfs);
    }
    return 0;
}

static MGVTBL arena_vtbl = {
    NULL, NULL, NULL, NULL, arena_cleanup
};

static SV* wrap_arena_internal(pTHX_ void* raw, bool is_tmpfs) {
    HV* hv = newHV();
    SV* ptr_sv = newSViv(PTR2IV(raw));
    hv_store(hv, "_arena_ptr", 10, ptr_sv, 0);
    if (is_tmpfs) {
        hv_store(hv, "_is_tmpfs", 9, newSViv(1), 0);
    }

    SV *rv = newRV_noinc((SV*)hv);
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    HV* stash = (reg && reg->stash_arena) ? reg->stash_arena : gv_stashpv("Protobuf::Arena", GV_ADD);
    sv_bless(rv, stash);

    sv_magicext((SV*)hv, NULL, PERL_MAGIC_ext, &arena_vtbl, (const char*)raw, 0);

    return rv;
}

SV *PerlUpb_Arena_New(pTHX) {
    void* raw = PerlUpb_Arena_CreateRaw(aTHX);
    return wrap_arena_internal(aTHX_ raw, false);
}

SV *PerlUpb_Arena_WrapRaw(pTHX_ upb_Arena* arena) {
    void* raw = PerlUpb_Arena_CreateRawFromUpb(aTHX_ arena);
    return wrap_arena_internal(aTHX_ raw, false);
}

upb_Arena *PerlUpb_Arena_Get(pTHX_ SV *sv) {
    if (!sv || !SvROK(sv) || !sv_isa(sv, "Protobuf::Arena")) {
        croak("Argument is not a blessed Protobuf::Arena object");
    }

    SV* rv = SvRV(sv);
    PerlUpb_Arena *arena_wrapper = NULL;

    if (SvTYPE(rv) == SVt_PVHV) {
        SV** svp = hv_fetch((HV*)rv, "_arena_ptr", 10, 0);
        if (svp && SvIOK(*svp)) {
            arena_wrapper = (PerlUpb_Arena*)INT2PTR(void*, SvIV(*svp));
        }
    }

    return arena_wrapper ? arena_wrapper->arena : NULL;
}

void PerlUpb_Arena_Destroy(pTHX_ SV *sv) {
    if (!sv || !SvROK(sv)) return;
    HV* hv = (HV*)SvRV(sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    if (!svp || !SvIOK(*svp)) return;
    void* ptr = INT2PTR(void*, SvIV(*svp));

    // Null out magic pointer to prevent arena_cleanup from double-freeing
    MAGIC* mg = mg_findext((SV*)hv, PERL_MAGIC_ext, &arena_vtbl);
    if (mg) mg->mg_ptr = NULL;

    SV** is_tmpfs = hv_fetch(hv, "_is_tmpfs", 9, 0);
    bool tmpfs = is_tmpfs && SvTRUE(*is_tmpfs);

    PerlUpb_Arena_DestroyRaw_Tmpfs(aTHX_ ptr, tmpfs);

    (void)hv_delete(hv, "_arena_ptr", 10, G_DISCARD);
}

void PerlUpb_Arena_Free(pTHX_ SV *sv) {
    PerlUpb_Arena_Destroy(aTHX_ sv);
}

void* PerlUpb_Arena_Detach(pTHX_ SV* sv) {
    if (!sv || !SvROK(sv) || !sv_isa(sv, "Protobuf::Arena")) {
        croak("Argument to detach() must be a Protobuf::Arena object");
    }
    HV* hv = (HV*)SvRV(sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    if (!svp || !SvIOK(*svp)) {
        return NULL;
    }
    void* raw = INT2PTR(void*, SvIV(*svp));

    // Disable the original object
    MAGIC* mg = mg_findext((SV*)hv, PERL_MAGIC_ext, &arena_vtbl);
    if (mg) mg->mg_ptr = NULL;
    (void)hv_delete(hv, "_arena_ptr", 10, G_DISCARD);

    return raw;
}

SV* PerlUpb_Arena_Attach(pTHX_ void* raw) {
    if (!raw) return &PL_sv_undef;
    // We assume it's a managed arena for now.
    // In a full implementation, we might need to know if it was tmpfs.
    return wrap_arena_internal(aTHX_ raw, false);
}

void PerlUpb_Arena_GetStats(pTHX_ SV *sv, PerlUpb_ArenaStats *stats) {
    if (!sv || !SvROK(sv) || !stats) return;
    memset(stats, 0, sizeof(PerlUpb_ArenaStats));

    HV* hv = (HV*)SvRV(sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    if (!svp || !SvIOK(*svp)) return;

    SV** is_tmpfs = hv_fetch(hv, "_is_tmpfs", 9, 0);
    if (is_tmpfs && SvTRUE(*is_tmpfs)) {
        PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)INT2PTR(void*, SvIV(*svp));
        stats->reserved = wrapper->alloc->size;
        stats->allocated = wrapper->alloc->offset;
        stats->blocks = 1;
    } else {
        PerlUpb_Arena* wrapper = (PerlUpb_Arena*)INT2PTR(void*, SvIV(*svp));
        stats->reserved = wrapper->stats_alloc.total_reserved;
        stats->allocated = stats->reserved;
        stats->blocks = wrapper->stats_alloc.total_blocks;
    }
}

uintptr_t PerlUpb_Arena_SpaceAllocated(pTHX_ SV *sv) {
    upb_Arena* arena = PerlUpb_Arena_Get(aTHX_ sv);
    if (!arena) return 0;
    return (uintptr_t)upb_Arena_SpaceAllocated(arena, NULL);
}

uintptr_t PerlUpb_Arena_SpaceReserved(pTHX_ SV *sv) {
    PerlUpb_ArenaStats stats;
    PerlUpb_Arena_GetStats(aTHX_ sv, &stats);
    return (uintptr_t)stats.reserved;
}

void PerlUpb_Arena_VerifyCanaries(pTHX_ SV *sv, const char* msg) {
}
