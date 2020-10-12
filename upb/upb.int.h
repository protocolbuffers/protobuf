
#ifndef UPB_INT_H_
#define UPB_INT_H_

#include "upb/upb.h"

struct mem_block;
typedef struct mem_block mem_block;

struct upb_arena {
  _upb_arena_head head;
  uint32_t *cleanups;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc *block_alloc;
  uint32_t last_size;

  /* When multiple arenas are fused together, each arena points to a parent
   * arena (root points to itself). The root tracks how many live arenas
   * reference it. */
  uint32_t refcount;  /* Only used when a->parent == a */
  struct upb_arena *parent;

  /* Linked list of blocks to free/cleanup. */
  mem_block *freelist, *freelist_tail;
};

#endif  /* UPB_INT_H_ */
