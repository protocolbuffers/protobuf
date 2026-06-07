#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "xs/descriptor_pool/pool.h"
#include "upb/mem/arena.h"
#include "upb/mem/alloc.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static bool _use_guard_pages() {
    static int val = -1;
    if (val == -1) {
        const char* env = getenv("PROTOBUF_PERL_USE_GUARD_PAGES");
        val = (env && *env != '0') ? 1 : 0;
    }
    return val == 1;
}

static size_t _page_size() {
    static size_t size = 0;
    if (size == 0) {
        size = (size_t)sysconf(_SC_PAGESIZE);
        if (size <= 0) size = 4096; // Fallback
    }
    return size;
}

// Generalized Linear allocator
static void* PerlUpb_BlockAlloc_Func(upb_alloc* alloc, void* ptr, size_t oldsize,
                                     size_t size, size_t* actual_size) {
    PerlUpb_BlockAlloc* b = (PerlUpb_BlockAlloc*)alloc;

    if (b->poisoned) return NULL;

    bool use_guard_pages = _use_guard_pages() && b->type == PERL_UPB_BLOCK_MALLOC;
    size_t page_size = _page_size();

    if (size == 0) { // FREE
        if (ptr != NULL) {
            if (use_guard_pages) {
                void* block_start = (char*)ptr - page_size;
                if (mprotect(block_start, b->mmap_size, PROT_READ | PROT_WRITE) != 0) {
                    // This is bad, but continuing munmap is probably the best course.
                    warn("mprotect failed during guard page free: %s", strerror(errno));
                }
                munmap(block_start, b->mmap_size);
            } else {
                PerlUpb_VerifyCanaries(ptr, oldsize, "BlockAlloc free", &b->poisoned);
                // For non-guard-page PERL_UPB_BLOCK_MALLOC, memory is part of b->region, freed with arena.
                // For PERL_UPB_BLOCK_MMAP, the whole region is unmapped in DestroyRaw_Tmpfs.
            }
        }
        return NULL;
    }

    if (ptr != NULL) { // REALLOC
        // Realloc is complex with guard pages, as the size changes.
        // For now, realloc with guard pages is not supported.
        if (use_guard_pages) {
            croak("Realloc not supported with guard pages in BlockAllocator");
        }
        PerlUpb_VerifyCanaries(ptr, oldsize, "BlockAlloc realloc", &b->poisoned);
        void* new_ptr = PerlUpb_BlockAlloc_Func(alloc, NULL, 0, size, actual_size);
        if (new_ptr && oldsize > 0) {
            memcpy(new_ptr, ptr, oldsize < size ? oldsize : size);
            // Free the old block - for BLOCK_MALLOC, this is a no-op as it's part of the single region
        }
        return new_ptr;
    }

    // ALLOC
    if (use_guard_pages) {
        size_t data_size = size;
        size_t total_alloc = page_size * 2 + data_size;
        b->mmap_size = total_alloc;

        void* mem = mmap(NULL, total_alloc, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) {
            warn("mmap failed for guard page alloc: %s", strerror(errno));
            return NULL;
        }

        if (mprotect(mem, page_size, PROT_NONE) != 0) {
            warn("mprotect failed for start guard page: %s", strerror(errno));
            munmap(mem, total_alloc);
            return NULL;
        }
        if (mprotect((char*)mem + page_size + data_size, page_size, PROT_NONE) != 0) {
            warn("mprotect failed for end guard page: %s", strerror(errno));
            munmap(mem, total_alloc);
            return NULL;
        }

        void* ret = (char*)mem + page_size;
        if (actual_size) *actual_size = data_size;
        b->region = mem; // Store the base mmap address
        b->size = data_size; // Store the usable size
        return ret;
    } else {
        // Original canary-based logic
        size_t requested_size = size + 2 * PERL_UPB_CANARY_SIZE;
        size_t aligned_size = (requested_size + 15) & ~15;
        if (b->offset + aligned_size > b->size) return NULL;

        void* raw = (char*)b->region + b->offset;
        PerlUpb_WriteCanaries(raw, size);
        void* ret = (char*)raw + PERL_UPB_CANARY_SIZE;

        b->offset += aligned_size;

        if (actual_size) *actual_size = size;
        return ret;
    }
}

SV* PerlUpb_Arena_NewTmpfs(pTHX_ const char* path, size_t size) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) croak("Failed to open tmpfs file %s: %s", path, strerror(errno));

    if (ftruncate(fd, size) != 0) {
        close(fd);
        croak("Failed to truncate tmpfs file %s: %s", path, strerror(errno));
    }

    void* region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        close(fd);
        croak("Failed to mmap tmpfs file %s: %s", path, strerror(errno));
    }

    PerlUpb_BlockAlloc* b_alloc = (PerlUpb_BlockAlloc*)safemalloc(sizeof(PerlUpb_BlockAlloc));
    b_alloc->base.func = PerlUpb_BlockAlloc_Func;
    b_alloc->type = PERL_UPB_BLOCK_MMAP;
    b_alloc->fd = fd;
    b_alloc->path = savepv(path);
    b_alloc->region = region;
    b_alloc->size = size;
    b_alloc->offset = 0;
    b_alloc->poisoned = false;
    b_alloc->mmap_size = size; // For tmpfs, mmap_size is the file size

    upb_Arena* arena = upb_Arena_Init(NULL, 0, &b_alloc->base);
    if (!arena) {
        munmap(region, size);
        close(fd);
        safefree(b_alloc->path);
        safefree(b_alloc);
        croak("Failed to initialize upb_Arena with block allocator");
    }

    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)safemalloc(sizeof(PerlUpb_Arena_Custom));
    wrapper->base.arena = arena;
    wrapper->alloc = b_alloc;

    HV* hv = newHV();
    hv_store(hv, "_arena_ptr", 10, newSViv(PTR2IV(wrapper)), 0);
    hv_store(hv, "_is_tmpfs", 9, newSViv(1), 0);

    SV* rv = newRV_noinc((SV*)hv);
    sv_bless(rv, gv_stashpv("Protobuf::Arena", GV_ADD));
    return rv;
}

// Internal helper for RAM-backed linear arenas
upb_Arena* PerlUpb_Arena_NewBlock(pTHX_ size_t size, PerlUpb_BlockAlloc** out_alloc) {
    PerlUpb_BlockAlloc* b_alloc = (PerlUpb_BlockAlloc*)safemalloc(sizeof(PerlUpb_BlockAlloc));
    b_alloc->base.func = PerlUpb_BlockAlloc_Func;
    b_alloc->type = PERL_UPB_BLOCK_MALLOC;
    b_alloc->fd = -1;
    b_alloc->path = NULL;
    b_alloc->poisoned = false;

    if (_use_guard_pages()) {
        // Defer allocation to the first BlockAlloc_Func call
        b_alloc->region = NULL;
        b_alloc->size = 0;
        b_alloc->offset = 0;
        b_alloc->mmap_size = 0;
    } else {
        void* region = safemalloc(size);
        b_alloc->region = region;
        b_alloc->size = size;
        b_alloc->offset = 0;
        b_alloc->mmap_size = size;
    }

    upb_Arena* arena = upb_Arena_Init(NULL, 0, &b_alloc->base);
    if (!arena) {
        if (b_alloc->region) safefree(b_alloc->region);
        safefree(b_alloc);
        return NULL;
    }

    if (out_alloc) *out_alloc = b_alloc;
    return arena;
}

SV* PerlUpb_Arena_AttachTmpfs(pTHX_ const char* path, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) croak("Failed to open existing tmpfs file %s: %s", path, strerror(errno));

    void* region = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        close(fd);
        croak("Failed to mmap tmpfs file (read-only) %s: %s", path, strerror(errno));
    }

    PerlUpb_BlockAlloc* b_alloc = (PerlUpb_BlockAlloc*)safemalloc(sizeof(PerlUpb_BlockAlloc));
    b_alloc->base.func = PerlUpb_BlockAlloc_Func;
    b_alloc->type = PERL_UPB_BLOCK_MMAP;
    b_alloc->fd = fd;
    b_alloc->path = savepv(path);
    b_alloc->region = region;
    b_alloc->size = size;
    b_alloc->offset = size;
    b_alloc->poisoned = false;
    b_alloc->mmap_size = size;

    upb_Arena* arena = upb_Arena_Init(NULL, 0, &upb_alloc_global);
    if (!arena) {
        munmap(region, size);
        close(fd);
        safefree(b_alloc->path);
        safefree(b_alloc);
        croak("Failed to create upb_Arena for attachment");
    }

    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)safemalloc(sizeof(PerlUpb_Arena_Custom));
    wrapper->base.arena = arena;
    wrapper->alloc = b_alloc;

    HV* hv = newHV();
    hv_store(hv, "_arena_ptr", 10, newSViv(PTR2IV(wrapper)), 0);
    hv_store(hv, "_is_tmpfs", 9, newSViv(1), 0);
    hv_store(hv, "_read_only", 10, newSViv(1), 0);

    SV* rv = newRV_noinc((SV*)hv);
    sv_bless(rv, gv_stashpv("Protobuf::Arena", GV_ADD));
    return rv;
}

void PerlUpb_Arena_DestroyRaw_Tmpfs(pTHX_ void* ptr, bool is_tmpfs) {
    if (!is_tmpfs) {
        PerlUpb_Arena_DestroyRaw(aTHX_ ptr);
        return;
    }

    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)ptr;
    if (wrapper) {
        if (wrapper->base.arena) PerlUpb_Arena_Release(aTHX_ wrapper->base.arena, PERL_UPB_LIFECYCLE_PERMANENT);
        if (wrapper->alloc) {
            if (wrapper->alloc->type == PERL_UPB_BLOCK_MMAP) {
                munmap(wrapper->alloc->region, wrapper->alloc->size);
                close(wrapper->alloc->fd);
                if (wrapper->alloc->path) safefree(wrapper->alloc->path);
            } else { // PERL_UPB_BLOCK_MALLOC
                if (_use_guard_pages()) {
                    // All allocations are separate mmaps, already freed by BlockAlloc_Func
                } else {
                    if (wrapper->alloc->region) safefree(wrapper->alloc->region);
                }
            }
            safefree(wrapper->alloc);
        }
        safefree(wrapper);
    }
}

bool PerlUpb_Arena_IsTmpfs(pTHX_ SV* arena_sv) {
    if (!arena_sv || !SvROK(arena_sv)) return false;
    HV* hv = (HV*)SvRV(arena_sv);
    SV** svp = hv_fetch(hv, "_is_tmpfs", 9, 0);
    return svp && SvIV(*svp);
}

const char* PerlUpb_Arena_GetPath(pTHX_ SV* arena_sv) {
    if (!PerlUpb_Arena_IsTmpfs(aTHX_ arena_sv)) return NULL;
    HV* hv = (HV*)SvRV(arena_sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    if (!svp) return NULL;
    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)SvIV(*svp);
    return wrapper->alloc->path;
}

bool PerlUpb_Arena_VerifySELinux(pTHX_ SV* arena_sv) {
    const char* path = PerlUpb_Arena_GetPath(aTHX_ arena_sv);
    if (!path) return true;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

size_t PerlUpb_Arena_GetOffset(pTHX_ SV* arena_sv, void* ptr) {
    if (!PerlUpb_Arena_IsTmpfs(aTHX_ arena_sv)) return 0;
    HV* hv = (HV*)SvRV(arena_sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    if (!svp) return 0;
    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)SvIV(*svp);
    if (ptr < wrapper->alloc->region || (char*)ptr >= (char*)wrapper->alloc->region + wrapper->alloc->size) {
        return 0;
    }
    return (char*)ptr - (char*)wrapper->alloc->region;
}

SV* PerlUpb_Arena_AttachMessage(pTHX_ SV* arena_sv, const char* name, size_t offset) {
    if (!PerlUpb_Arena_IsTmpfs(aTHX_ arena_sv)) croak("Arena is not a tmpfs arena");
    HV* hv = (HV*)SvRV(arena_sv);
    SV** svp = hv_fetch(hv, "_arena_ptr", 10, 0);
    PerlUpb_Arena_Custom* wrapper = (PerlUpb_Arena_Custom*)SvIV(*svp);
    if (offset >= wrapper->alloc->size) croak("Offset %zu out of bounds for arena size %zu", offset, wrapper->alloc->size);
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ PerlUpb_DescriptorPool_GeneratedPool(aTHX));
    const upb_MessageDef* mdef = upb_DefPool_FindMessageByName(pool, name);
    if (!mdef) croak("Message definition not found: %s", name);
    upb_Message* msg = (upb_Message*)((char*)wrapper->alloc->region + offset);
    return PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_sv, 0);
}
