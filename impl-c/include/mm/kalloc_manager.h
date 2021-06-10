#pragma once

#include "mm/alloc.h"
#include "mm/frame.h"
#include "mm/startup.h"

#include "list.h"
#include <stdint.h>

/**
 * Allocation Manager:
 *  On charge of dynamic memory allocation by using frame/object allocators
 *
 * Caution: must initialize the startup allocator before using
 * any of the function listed below, otherwise the buddy system
 * wont initialize properly
 * */

extern struct AllocationManager KAllocManager;
typedef struct AllocationManager {
  SlabAllocator obj_allocator_list[SLAB_NUM_SLAB_SIZES];
  BuddyAllocater frame_allocator;
} AllocationManager;

// Statically linked to the heap space
// because their lifetimes is equal to the system itself
extern struct Frame Frames[1 << BUDDY_MAX_EXPONENT];