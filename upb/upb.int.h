
#ifndef UPB_INT_H_
#define UPB_INT_H_

#include "upb/upb.h"

struct mem_block;
typedef struct mem_block mem_block;

struct upb_arena {
  _upb_arena_head head;
  /* Stores cleanup metadata for this arena.
   * - a pointer to the current cleanup counter.
   * - a boolean indicating if there is an unowned initial block.  */
  uintptr_t cleanup_metadata;

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
