#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/protobuf/registry.h"

#define REGISTRY_KEY "Protobuf::Registry"

static int registry_cleanup(pTHX_ SV* sv, MAGIC* mg) {
    if (PL_dirty) return 0; // Let Perl handle it
    PerlUpb_Registry* reg = INT2PTR(PerlUpb_Registry*, SvIV(sv));
    if (reg) {
        // We don't SvREFCNT_dec these here because global destruction
        // is already handling it, and we might trigger double-frees.
        // We just need to free the struct itself.
        safefree(reg);
        sv_setiv(sv, 0);
    }
    return 0;
}

static MGVTBL registry_vtbl = {
    NULL, NULL, NULL, NULL, registry_cleanup
};

void PerlUpb_Registry_Init(pTHX) {
    SV** svp = hv_fetch(PL_modglobal, REGISTRY_KEY, strlen(REGISTRY_KEY), 1);
    if (!svp) {
        croak("Failed to initialize Protobuf registry in PL_modglobal");
    }

    if (!SvIOK(*svp)) {
        PerlUpb_Registry* reg = (PerlUpb_Registry*)safemalloc(sizeof(PerlUpb_Registry));
        memset(reg, 0, sizeof(PerlUpb_Registry));

        // Default capacity
        reg->max_cache_capacity = 100000;
        reg->cached_transient_arena = NULL;
        reg->descriptor_fingerprints = newHV();
        reg->stash_cache = newHV();

        reg->stash_message = gv_stashpv("Protobuf::Message", GV_ADD);
        reg->stash_repeated = gv_stashpv("Protobuf::Internal::Repeated", GV_ADD);
        reg->stash_map = gv_stashpv("Protobuf::Internal::Map", GV_ADD);
        reg->stash_arena = gv_stashpv("Protobuf::Arena", GV_ADD);
        reg->stash_unknown_fields = gv_stashpv("Protobuf::UnknownFieldSet", GV_ADD);
        reg->stash_repeated_public = gv_stashpv("Protobuf::Internal::Repeated::Public", GV_ADD);
        reg->stash_map_public = gv_stashpv("Protobuf::Internal::Map::Public", GV_ADD);

        // Chaos defaults
        reg->chaos.enabled = false;
        reg->chaos.fail_probability = 0.0;
        reg->chaos.delay_probability = 0.0;
        reg->chaos.max_delay_ms = 0;
        reg->chaos.seed = (unsigned int)time(NULL);

        sv_setiv(*svp, PTR2IV(reg));

        // Add magic for automated cleanup during global destruction
        sv_magicext(*svp, NULL, PERL_MAGIC_ext, &registry_vtbl, (const char*)NULL, 0);
    }
}

PerlUpb_Registry* PerlUpb_Registry_Get(pTHX) {
    if (PL_dirty) return NULL;

    SV** svp = hv_fetch(PL_modglobal, REGISTRY_KEY, strlen(REGISTRY_KEY), 0);
    if (svp && SvIOK(*svp)) {
        return INT2PTR(PerlUpb_Registry*, SvIV(*svp));
    }

    // Auto-init if missing (lazy)
    PerlUpb_Registry_Init(aTHX);
    svp = hv_fetch(PL_modglobal, REGISTRY_KEY, strlen(REGISTRY_KEY), 0);
    return INT2PTR(PerlUpb_Registry*, SvIV(*svp));
}

void PerlUpb_Registry_PreallocateArena(pTHX) {
    if (PL_dirty) return;
    PerlUpb_Registry* reg = PerlUpb_Registry_Get(aTHX);
    if (!reg) return;

    if (reg->idle_time_arena_hook) {
        reg->idle_time_arena_hook(aTHX);
    }

    if (!reg->cached_transient_arena) {
        reg->cached_transient_arena = upb_Arena_New();
    }
}
