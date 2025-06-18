use std::alloc::{alloc, dealloc, Layout};
use std::ptr::{self, NonNull};
use std::sync::{Arc, Mutex, MutexGuard, PoisonError}; // Needed for tracking blocks in FuseGroup
                                                      // Default alignment for upb arenas is typically 2 * sizeof(void*)
                                                      // On 64-bit, this is 16. On 32-bit, this is 8.

pub const UPB_ARENA_ALIGNMENT: usize = std::mem::size_of::<*mut ()>() * 2;

const DEFAULT_MIN_BLOCK_SIZE: usize = 4096; // A common page size

/// Represents a single block of memory allocated by the arena system.
/// Now managed by the FuseGroup.
#[derive(Debug)]
struct Block {
    ptr: NonNull<u8>, // Pointer to the start of the allocated memory
    layout: Layout,   // Layout used for allocation (size and alignment)
}

impl Drop for Block {
    fn drop(&mut self) {
        // Safety: ptr and layout were used for allocation and are valid.
        unsafe {
            dealloc(self.ptr.as_ptr(), self.layout);
        }
    }
}

/// Data shared by all arenas in a fused group.
#[derive(Debug)]
struct FuseGroupData {
    // Using Mutex for interior mutability shared across Arcs.
    // Blocks owned by *this specific fuse group*. Does not include external initial blocks.
    owned_blocks: Vec<Block>,
    space_allocated_by_group: usize, // Bytes allocated from system for blocks in owned_blocks
    ref_count: usize,                // How many UpbArena Arcs point to this group
}

/// Internal Rust representation of `upb_Arena`. Renamed from RustUpbArena.
pub struct UpbArena {
    // State specific to this arena instance
    current_block_idx: usize,   // Index into the *fuse group's* block list
    current_alloc_ptr: *mut u8, // Next byte to allocate from in the current block
    current_block_end: *mut u8, // End of the current block's usable memory

    // Information about the initial block, if provided externally
    initial_block_ptr: *mut u8, // Pointer to the start of the initial block (not owned)
    initial_block_end: *mut u8, // Pointer to the end of the initial block

    // Shared state for the fuse group this arena belongs to
    fuse_group: Arc<Mutex<FuseGroupData>>,
}

// Custom Debug impl to avoid recursive lock attempts if printing fuse_group directly
impl std::fmt::Debug for UpbArena {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let fuse_group_ptr = Arc::as_ptr(&self.fuse_group);
        f.debug_struct("UpbArena")
            .field("current_block_idx", &self.current_block_idx)
            .field("current_alloc_ptr", &self.current_alloc_ptr)
            .field("current_block_end", &self.current_block_end)
            .field("initial_block_ptr", &self.initial_block_ptr)
            .field("initial_block_end", &self.initial_block_end)
            .field("fuse_group (ptr)", &fuse_group_ptr) // Avoid locking in Debug
            .finish()
    }
}

impl UpbArena {
    /// Creates a new, empty arena that will allocate its own blocks.
    pub fn new() -> Self {
        let initial_group_data = FuseGroupData {
            owned_blocks: Vec::new(),
            space_allocated_by_group: 0,
            ref_count: 1, // This new arena holds the first reference
        };
        UpbArena {
            // blocks field removed, now owned by FuseGroupData
            current_block_idx: 0, // Will point to the first block when allocated
            current_alloc_ptr: ptr::null_mut(),
            current_block_end: ptr::null_mut(),
            initial_block_ptr: ptr::null_mut(),
            initial_block_end: ptr::null_mut(),
            fuse_group: Arc::new(Mutex::new(initial_group_data)),
        }
    }

    /// Initializes an arena with an initial block of memory.
    /// If `mem` is NULL or `n` is 0, this behaves like `new()`.
    /// The `alloc_func` from C's `upb_alloc` is ignored; Rust's global allocator is used.
    /// The initial block `mem` is *not* owned or freed by the arena/fuse group.
    ///
    /// # Safety
    /// If `n > 0`, `mem` must be a valid pointer to `n` bytes of memory.
    /// The lifetime of this initial block must outlive the arena.
    pub unsafe fn init(mem: *mut u8, n: usize /*, _alloc_func: C_upb_alloc_func */) -> Self {
        let initial_group_data =
            FuseGroupData { owned_blocks: Vec::new(), space_allocated_by_group: 0, ref_count: 1 };
        let mut arena = UpbArena {
            current_block_idx: usize::MAX, // Special index for initial block
            current_alloc_ptr: ptr::null_mut(),
            current_block_end: ptr::null_mut(),
            initial_block_ptr: ptr::null_mut(),
            initial_block_end: ptr::null_mut(),
            fuse_group: Arc::new(Mutex::new(initial_group_data)),
        };

        if !mem.is_null() && n > 0 {
            arena.initial_block_ptr = mem;
            arena.initial_block_end = mem.add(n);
            arena.current_alloc_ptr = arena.initial_block_ptr;
            arena.current_block_end = arena.initial_block_end;
        } else {
            // No initial block, behave like new() regarding current pointers
            arena.current_block_idx = 0; // Ready for first allocation
        }
        arena
    }

    /// Allocates `size` bytes with appropriate alignment.
    /// Returns NULL on allocation failure.
    pub fn malloc(&mut self, size: usize) -> *mut u8 {
        if size == 0 {
            return ptr::null_mut();
        }

        let align = UPB_ARENA_ALIGNMENT;
        // Calculate aligned size needed
        let layout_result = Layout::from_size_align(size, align);
        let alloc_layout = match layout_result {
            Ok(l) => l,
            Err(_) => return ptr::null_mut(), // Invalid size/align combination
        };
        let alloc_size = alloc_layout.size(); // Use size from layout for alignment padding

        // Try to allocate from the current block (initial or owned)
        // Safety: Pointer arithmetic is okay if pointers are valid/null
        let available_in_current = if self.current_alloc_ptr.is_null() {
            0
        } else {
            (self.current_block_end as usize).saturating_sub(self.current_alloc_ptr as usize)
        };

        if alloc_size <= available_in_current {
            let ptr = self.current_alloc_ptr;
            // Safety: We checked bounds, pointer arithmetic within the block
            self.current_alloc_ptr = unsafe { self.current_alloc_ptr.add(alloc_size) };
            return ptr;
        }

        // Need a new block. Lock the fuse group to add a block.
        let mut fuse_group_guard = self.fuse_group.lock().unwrap_or_else(|e| e.into_inner());

        let new_block_size = std::cmp::max(alloc_size, DEFAULT_MIN_BLOCK_SIZE);
        // Ensure the block layout uses the required arena alignment
        let block_layout = match Layout::from_size_align(new_block_size, align) {
            Ok(layout) => layout,
            Err(_) => return ptr::null_mut(),
        };

        // Safety: `alloc` can return null on OOM.
        let block_ptr = unsafe { alloc(block_layout) };
        if block_ptr.is_null() {
            // Don't call handle_alloc_error which panics. Return null as per C API expectation.
            return ptr::null_mut();
        }

        // Store the new block in the shared fuse group
        fuse_group_guard.owned_blocks.push(Block {
            ptr: NonNull::new(block_ptr).unwrap(), // We checked for null
            layout: block_layout,
        });
        fuse_group_guard.space_allocated_by_group += new_block_size;

        // Update this arena instance's pointers to the new block
        self.current_block_idx = fuse_group_guard.owned_blocks.len() - 1;
        self.current_alloc_ptr = block_ptr;
        // Safety: Pointer arithmetic within the newly allocated block
        self.current_block_end = unsafe { block_ptr.add(new_block_size) };

        // Allocate the requested size from the start of the new block
        let result_ptr = self.current_alloc_ptr;
        // Safety: Pointer arithmetic, alloc_size <= new_block_size
        self.current_alloc_ptr = unsafe { self.current_alloc_ptr.add(alloc_size) };
        result_ptr
    }

    /// Fuses the lifetimes of two arenas.
    /// After fusing, memory allocated in either arena will only be freed
    /// when *all* arenas originally in either group are freed.
    /// Returns true on success, false on failure (e.g., lock poisoning).
    pub fn fuse(a: &mut UpbArena, b: &mut UpbArena) -> bool {
        // Avoid self-fusing (initial check)
        if Arc::ptr_eq(&a.fuse_group, &b.fuse_group) {
            return true;
        }

        // Lock both fuse groups. Self::lock_both should handle locking order
        // and return guards corresponding to `a.fuse_group` and `b.fuse_group` respectively.
        let (mut guard_a, mut guard_b) = match Self::lock_both(&a.fuse_group, &b.fuse_group) {
            Ok(guards) => guards,
            Err(_) => return false, // Lock poisoning or other error from lock_both
        };

        // If they are already the same group after locking (e.g., another thread fused them
        // and updated a.fuse_group or b.fuse_group before we acquired our locks,
        // or if lock_both detected they became same).
        // This check might be redundant if lock_both already panics/errors on same underlying mutex.
        // If lock_both can successfully lock two Arcs that point to different Arc objects
        // but those Arc objects then point to the *same* Mutex due to a concurrent fuse,
        // then this check is relevant.
        // However, the more direct check is if the *guards* refer to the same data.
        // For now, keeping the original check on the Arc pointers of 'a' and 'b'.
        if Arc::ptr_eq(&a.fuse_group, &b.fuse_group) {
            return true;
        }

        // Determine which arena's fuse_group will be updated and what Arc it will point to.
        enum UpdateAction {
            UpdateAWithValue(Arc<Mutex<FuseGroupData>>),
            UpdateBWithValue(Arc<Mutex<FuseGroupData>>),
        }
        let action: UpdateAction;

        if guard_a.owned_blocks.len() >= guard_b.owned_blocks.len() {
            // Merge B into A's group. Arena B's fuse_group will point to A's fuse_group.
            let group_to_keep_data = &mut *guard_a;
            let group_to_drain_data = &mut *guard_b;

            // Perform data merge
            let blocks_to_move = std::mem::take(&mut group_to_drain_data.owned_blocks);
            group_to_keep_data.owned_blocks.extend(blocks_to_move);
            group_to_keep_data.space_allocated_by_group +=
                group_to_drain_data.space_allocated_by_group;
            group_to_keep_data.ref_count += group_to_drain_data.ref_count; // Transfer ref_counts

            // Clear data from the drained group
            group_to_drain_data.space_allocated_by_group = 0;
            group_to_drain_data.ref_count = 0;

            // Arena 'b' will be updated to point to `a.fuse_group`.
            // Clone the Arc that `a.fuse_group` currently refers to.
            action = UpdateAction::UpdateBWithValue(Arc::clone(&a.fuse_group));
        } else {
            // Merge A into B's group. Arena A's fuse_group will point to B's fuse_group.
            let group_to_keep_data = &mut *guard_b;
            let group_to_drain_data = &mut *guard_a;

            // Perform data merge
            let blocks_to_move = std::mem::take(&mut group_to_drain_data.owned_blocks);
            group_to_keep_data.owned_blocks.extend(blocks_to_move);
            group_to_keep_data.space_allocated_by_group +=
                group_to_drain_data.space_allocated_by_group;
            group_to_keep_data.ref_count += group_to_drain_data.ref_count; // Transfer ref_counts

            // Clear data from the drained group
            group_to_drain_data.space_allocated_by_group = 0;
            group_to_drain_data.ref_count = 0;

            // Arena 'a' will be updated to point to `b.fuse_group`.
            action = UpdateAction::UpdateAWithValue(Arc::clone(&b.fuse_group));
        }

        // IMPORTANT: Drop the guards *before* assigning to `a.fuse_group` or `b.fuse_group`.
        // This releases the borrows on the Mutexes.
        drop(guard_a);
        drop(guard_b);

        // Now, perform the assignment. This is safe as borrows are released.
        match action {
            UpdateAction::UpdateAWithValue(new_arc) => {
                a.fuse_group = new_arc;
            }
            UpdateAction::UpdateBWithValue(new_arc) => {
                b.fuse_group = new_arc;
            }
        }

        true
    }

    fn lock_both<'a>(
        arc1: &'a Arc<Mutex<FuseGroupData>>,
        arc2: &'a Arc<Mutex<FuseGroupData>>,
    ) -> Result<
        (MutexGuard<'a, FuseGroupData>, MutexGuard<'a, FuseGroupData>),
        PoisonError<MutexGuard<'a, FuseGroupData>>,
    > {
        let ptr1 = Arc::as_ptr(arc1);
        let ptr2 = Arc::as_ptr(arc2);

        // Critical: Prevent deadlock if both Arcs point to the same Mutex.
        if ptr1 == ptr2 {
            panic!("lock_both_fixed: arc1 and arc2 point to the same Mutex. This would deadlock.");
        }

        let guard_a: MutexGuard<'a, FuseGroupData>;
        let guard_b: MutexGuard<'a, FuseGroupData>;
        let swapped_for_return: bool;

        // Determine locking order based on memory address to prevent deadlocks.
        if ptr1 < ptr2 {
            guard_a = arc1.lock()?;
            // If second lock fails, guard_a is automatically dropped.
            guard_b = arc2.lock()?;
            swapped_for_return = false;
        } else {
            // ptr1 > ptr2 (equality handled by panic above)
            guard_a = arc2.lock()?; // Lock arc2 first (lower address)
                                    // If second lock fails, guard_a (from arc2) is automatically dropped.
            guard_b = arc1.lock()?; // Then lock arc1
            swapped_for_return = true;
        }

        // Return guards in the original order requested by the caller.
        if swapped_for_return {
            Ok((guard_b, guard_a)) // guard_b corresponds to arc1, guard_a to arc2
        } else {
            Ok((guard_a, guard_b)) // guard_a corresponds to arc1, guard_b to arc2
        }
    }

    /// Returns the total bytes allocated from the system for all blocks
    /// managed by this arena's fuse group.
    pub fn space_allocated(&self) -> usize {
        // Lock and read the shared value
        match self.fuse_group.lock() {
            Ok(guard) => guard.space_allocated_by_group,
            Err(_) => 0, // Or panic on poisoned lock? For C API, returning 0 might be safer.
        }
    }

    /// Returns an estimate of space used within allocated blocks.
    /// M1 SIMPLIFICATION: This only calculates the space used within the *current block*
    /// being used by *this specific arena instance*. It does not sum previously filled
    /// blocks and does not accurately represent total usage across a fused group.
    /// A more accurate implementation matching C upb's logic (summing full sizes of
    /// previous blocks in this arena's view + used part of current block) or a
    /// true high-watermark tracking across the fuse group is needed for full fidelity.
    pub fn space_used(&self) -> usize {
        let current_block_start_ptr = if self.current_block_idx != usize::MAX {
            // Block is owned by the fuse group
            match self.fuse_group.lock() {
                Ok(guard) => {
                    // Check if index is valid *within the locked guard*
                    if self.current_block_idx < guard.owned_blocks.len() {
                        guard.owned_blocks[self.current_block_idx].ptr.as_ptr()
                    } else {
                        // This might happen if blocks were allocated but this arena instance
                        // hasn't advanced its current_block_idx yet, or if fused.
                        // Fall back to checking the initial block or zero.
                        if !self.initial_block_ptr.is_null() {
                            self.initial_block_ptr
                        } else {
                            ptr::null_mut()
                        }
                    }
                }
                Err(_) => ptr::null_mut(), // Poisoned lock
            }
        } else if !self.initial_block_ptr.is_null() {
            // Using the external initial block
            self.initial_block_ptr
        } else {
            ptr::null_mut() // Arena is fresh, no blocks allocated yet
        };

        if !self.current_alloc_ptr.is_null() && !current_block_start_ptr.is_null() {
            (self.current_alloc_ptr as usize).saturating_sub(current_block_start_ptr as usize)
        } else {
            0
        }
    }
}

impl Drop for UpbArena {
    fn drop(&mut self) {
        // Lock the fuse group to decrement the reference count.
        if let Ok(mut guard) = self.fuse_group.lock() {
            guard.ref_count -= 1;
            if guard.ref_count == 0 {
                // This was the last arena referencing this group.
                // We take ownership of the blocks from the FuseGroupData.
                // The FuseGroupData itself will be dropped when the Arc's strong count becomes 0.
                // We need to move the blocks out *while holding the lock* to prevent races.
                let blocks_to_free = std::mem::take(&mut guard.owned_blocks);

                // Drop the guard *before* freeing blocks to release the mutex.
                drop(guard);

                // Now, actually drop the blocks (which calls their Drop impl -> dealloc)
                // This happens outside the lock.
                drop(blocks_to_free);
            }
            // else: Other arenas still reference this group, do nothing with blocks.
        } else {
            // Mutex was poisoned. This indicates a panic occurred while a lock was held.
            // Memory leakage of the blocks might occur, but trying to recover could be unsafe.
            // Log an error or handle appropriately based on project policy.
            eprintln!("UpbArena fuse group mutex was poisoned during drop!");
        }

        // Note: The initial_block (if provided via `upb_Arena_Init` from external memory)
        // is not owned by the fuse group, so it's not freed here.
    }
}
