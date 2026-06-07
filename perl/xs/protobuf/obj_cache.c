#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "xs/protobuf/obj_cache.h"
#include "xs/protobuf/registry.h"
#include "xs/protobuf/port.h"

#define NUM_CACHE_STRIPES 16
#define AUDIT_LOG_SIZE 2048

typedef struct {
    int type;
    const void* ptr;
    time_t timestamp;
} audit_entry_t;

static struct {
    audit_entry_t entries[AUDIT_LOG_SIZE];
    size_t head;
    size_t count;
    PERL_PROTOBUF_MUTEX_T mutex;
    int init;
} global_audit_log = { .init = 0 };

static PERL_PROTOBUF_MUTEX_T cache_mutexes[NUM_CACHE_STRIPES];
static PERL_PROTOBUF_MUTEX_T lru_mutex;
static int cache_mutexes_init = 0;
#ifdef USE_ITHREADS
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef struct {
    uint64_t acquisitions;
    uint64_t contentions;
} contention_stat_t;

static struct {
    contention_stat_t stripes[NUM_CACHE_STRIPES];
    contention_stat_t lru;
    contention_stat_t audit;
} contention_stats;

static void LOCK_AND_PROFILE(PERL_PROTOBUF_MUTEX_T* m, contention_stat_t* s) {
#ifdef USE_ITHREADS
    if (PERL_PROTOBUF_MUTEX_TRYLOCK(m)) {
        s->acquisitions++;
    } else {
        s->contentions++;
        s->acquisitions++;
        PERL_PROTOBUF_MUTEX_LOCK(m);
    }
#else
    s->acquisitions++;
#endif
}

static void ensure_mutexes_init(void) {
    if (cache_mutexes_init) return;
#ifdef USE_ITHREADS
    pthread_mutex_lock(&init_mutex);
    if (cache_mutexes_init) {
        pthread_mutex_unlock(&init_mutex);
        return;
    }
#endif
    for (int i = 0; i < NUM_CACHE_STRIPES; i++) {
        PERL_PROTOBUF_MUTEX_INIT(&cache_mutexes[i]);
    }
    PERL_PROTOBUF_MUTEX_INIT(&lru_mutex);
    PERL_PROTOBUF_MUTEX_INIT(&global_audit_log.mutex);
    memset(&contention_stats, 0, sizeof(contention_stats));
    global_audit_log.init = 1;
    cache_mutexes_init = 1;
#ifdef USE_ITHREADS
    pthread_mutex_unlock(&init_mutex);
#endif
}

static inline int get_stripe(const void* ptr) {
    uintptr_t val = (uintptr_t)ptr;
    return (int)((val >> 4) % NUM_CACHE_STRIPES);
}

// THX-Free Logger
void PerlUpb_ObjCache_LogEvent(int type, const void* ptr) {
    if (!global_audit_log.init) return;

    PERL_PROTOBUF_MUTEX_LOCK(&global_audit_log.mutex);
    size_t idx = (global_audit_log.head + global_audit_log.count) % AUDIT_LOG_SIZE;
    if (global_audit_log.count == AUDIT_LOG_SIZE) {
        global_audit_log.head = (global_audit_log.head + 1) % AUDIT_LOG_SIZE;
    } else {
        global_audit_log.count++;
    }
    global_audit_log.entries[idx].type = type;
    global_audit_log.entries[idx].ptr = ptr;
    global_audit_log.entries[idx].timestamp = time(NULL);
    PERL_PROTOBUF_MUTEX_UNLOCK(&global_audit_log.mutex);
}

void PerlUpb_ObjCache_Init(pTHX) {
    ensure_mutexes_init();
    // Cache the HV/AV in the registry for fast access
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (!reg->obj_cache) {
        SV* cache_sv = get_sv("Protobuf::_obj_cache", GV_ADD);
        if (!SvROK(cache_sv)) {
            reg->obj_cache = newHV();
            sv_setsv(cache_sv, newRV_noinc((SV*)reg->obj_cache));
        } else {
            reg->obj_cache = (HV*)SvRV(cache_sv);
        }
    }
    if (!reg->obj_lru) {
        SV* lru_sv = get_sv("Protobuf::_obj_lru", GV_ADD);
        if (!SvROK(lru_sv)) {
            reg->obj_lru = newAV();
            sv_setsv(lru_sv, newRV_noinc((SV*)reg->obj_lru));
        } else {
            reg->obj_lru = (AV*)SvRV(lru_sv);
        }
    }
}

#define CACHE_KEY_LEN sizeof(void*)

void PerlUpb_ObjCache_SetCapacity(pTHX_ size_t capacity) {
    ensure_mutexes_init();
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    LOCK_AND_PROFILE(&lru_mutex, &contention_stats.lru);
    reg->max_cache_capacity = capacity;
    PERL_PROTOBUF_MUTEX_UNLOCK(&lru_mutex);
}

size_t PerlUpb_ObjCache_GetCapacity(pTHX) {
    return PerlUpb_Registry_Get(aTHX)->max_cache_capacity;
}

void PerlUpb_ObjCache_Add(pTHX_ const void* ptr, SV* obj) {
    if (!ptr || !obj || !SvROK(obj)) return;
    ensure_mutexes_init();
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (!reg) return;

    int stripe = get_stripe(ptr);
    LOCK_AND_PROFILE(&cache_mutexes[stripe], &contention_stats.stripes[stripe]);

    HV* cache = reg->obj_cache;
    char key[CACHE_KEY_LEN];
    memcpy(key, &ptr, CACHE_KEY_LEN);

    SV* target = SvRV(obj);
    SV* weak_rv = newRV_inc(target);
    sv_rvweaken(weak_rv);

    if (!hv_store(cache, key, CACHE_KEY_LEN, weak_rv, 0)) {
        SvREFCNT_dec(weak_rv);
    }

    PERL_PROTOBUF_MUTEX_UNLOCK(&cache_mutexes[stripe]);
    PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_ADD, ptr);

    LOCK_AND_PROFILE(&lru_mutex, &contention_stats.lru);
    AV* lru = reg->obj_lru;
    av_push(lru, newSVpvn(key, CACHE_KEY_LEN));

    while ((size_t)av_len(lru) + 1 > reg->max_cache_capacity) {
        SV* oldest_key_sv = av_shift(lru);
        if (oldest_key_sv && SvOK(oldest_key_sv)) {
            STRLEN len;
            const char* oldest_key = SvPVbyte(oldest_key_sv, len);
            if (len == CACHE_KEY_LEN) {
                void* evict_ptr;
                memcpy(&evict_ptr, oldest_key, CACHE_KEY_LEN);
                int evict_stripe = get_stripe(evict_ptr);
                LOCK_AND_PROFILE(&cache_mutexes[evict_stripe], &contention_stats.stripes[evict_stripe]);
                hv_delete(cache, oldest_key, len, G_DISCARD);
                PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_EVICT, evict_ptr);
                PERL_PROTOBUF_MUTEX_UNLOCK(&cache_mutexes[evict_stripe]);
            }
        }
        if (oldest_key_sv && !PL_dirty) SvREFCNT_dec(oldest_key_sv);
    }
    PERL_PROTOBUF_MUTEX_UNLOCK(&lru_mutex);
}

SV* PerlUpb_ObjCache_Get(pTHX_ const void* ptr) {
    if (!ptr) return NULL;
    ensure_mutexes_init();
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (!reg) return NULL;

    int stripe = get_stripe(ptr);
    LOCK_AND_PROFILE(&cache_mutexes[stripe], &contention_stats.stripes[stripe]);

    HV* cache = reg->obj_cache;
    char key[CACHE_KEY_LEN];
    memcpy(key, &ptr, CACHE_KEY_LEN);

    SV** svp = hv_fetch(cache, key, CACHE_KEY_LEN, 0);
    SV* result = NULL;

    if (svp) {
        SV* rv = *svp;
        if (rv && SvROK(rv)) {
            SV* target = SvRV(rv);
            if (target && target != &PL_sv_undef) {
                result = newRV_inc(target);
                PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_HIT, ptr);
            }
        }
        if (!result) {
            hv_delete(cache, key, CACHE_KEY_LEN, G_DISCARD);
            PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_MISS, ptr);
        }
    } else {
        PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_MISS, ptr);
    }

    PERL_PROTOBUF_MUTEX_UNLOCK(&cache_mutexes[stripe]);
    return result;
}

void PerlUpb_ObjCache_Delete(pTHX_ const void* ptr) {
    if (!ptr) return;
    ensure_mutexes_init();
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (!reg) return;

    int stripe = get_stripe(ptr);
    LOCK_AND_PROFILE(&cache_mutexes[stripe], &contention_stats.stripes[stripe]);

    char key[CACHE_KEY_LEN];
    memcpy(key, &ptr, CACHE_KEY_LEN);

    if (hv_exists(reg->obj_cache, key, CACHE_KEY_LEN)) {
        hv_delete(reg->obj_cache, key, CACHE_KEY_LEN, G_DISCARD);
        PerlUpb_ObjCache_LogEvent(OBJ_CACHE_EVENT_DELETE, ptr);
    }

    PERL_PROTOBUF_MUTEX_UNLOCK(&cache_mutexes[stripe]);
}

void PerlUpb_ObjCache_DeleteEntry(pTHX_ const char* key_bytes, STRLEN len) {
    if (!key_bytes || len != CACHE_KEY_LEN) return;
    void* ptr;
    memcpy(&ptr, key_bytes, CACHE_KEY_LEN);
    PerlUpb_ObjCache_Delete(aTHX_ ptr);
}

void PerlUpb_ObjCache_Clear(pTHX) {
    ensure_mutexes_init();
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);

    for (int i = 0; i < NUM_CACHE_STRIPES; i++) {
        LOCK_AND_PROFILE(&cache_mutexes[i], &contention_stats.stripes[i]);
    }
    LOCK_AND_PROFILE(&lru_mutex, &contention_stats.lru);

    if (!PL_dirty) {
        if (reg->obj_cache) hv_clear(reg->obj_cache);
        if (reg->obj_lru) av_clear(reg->obj_lru);
    }

    PERL_PROTOBUF_MUTEX_LOCK(&global_audit_log.mutex);
    global_audit_log.head = 0;
    global_audit_log.count = 0;
    PERL_PROTOBUF_MUTEX_UNLOCK(&global_audit_log.mutex);

    PERL_PROTOBUF_MUTEX_UNLOCK(&lru_mutex);
    for (int i = NUM_CACHE_STRIPES - 1; i >= 0; i--) {
        PERL_PROTOBUF_MUTEX_UNLOCK(&cache_mutexes[i]);
    }
}

SV* PerlUpb_ObjCache_GetAuditLog(pTHX) {
    ensure_mutexes_init();
    PERL_PROTOBUF_MUTEX_LOCK(&global_audit_log.mutex);
    AV* av = newAV();
    for (size_t i = 0; i < global_audit_log.count; i++) {
        size_t idx = (global_audit_log.head + i) % AUDIT_LOG_SIZE;
        HV* entry_hv = newHV();
        hv_store(entry_hv, "type", 4, newSViv(global_audit_log.entries[idx].type), 0);
        char ptr_buf[64];
        sprintf(ptr_buf, "%p", global_audit_log.entries[idx].ptr);
        hv_store(entry_hv, "ptr", 3, newSVpv(ptr_buf, 0), 0);
        hv_store(entry_hv, "timestamp", 9, newSViv(global_audit_log.entries[idx].timestamp), 0);
        av_push(av, newRV_noinc((SV*)entry_hv));
    }
    PERL_PROTOBUF_MUTEX_UNLOCK(&global_audit_log.mutex);
    return newRV_noinc((SV*)av);
}

SV* PerlUpb_ObjCache_GetContentionStats(pTHX) {
    ensure_mutexes_init();
    HV* hv = newHV();

    AV* stripes_av = newAV();
    for (int i = 0; i < NUM_CACHE_STRIPES; i++) {
        HV* s_hv = newHV();
        hv_store(s_hv, "acquisitions", 12, newSVuv(contention_stats.stripes[i].acquisitions), 0);
        hv_store(s_hv, "contentions", 11, newSVuv(contention_stats.stripes[i].contentions), 0);
        av_push(stripes_av, newRV_noinc((SV*)s_hv));
    }
    hv_store(hv, "stripes", 7, newRV_noinc((SV*)stripes_av), 0);

    HV* lru_hv = newHV();
    hv_store(lru_hv, "acquisitions", 12, newSVuv(contention_stats.lru.acquisitions), 0);
    hv_store(lru_hv, "contentions", 11, newSVuv(contention_stats.lru.contentions), 0);
    hv_store(hv, "lru", 3, newRV_noinc((SV*)lru_hv), 0);

    return newRV_noinc((SV*)hv);
}
