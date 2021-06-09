#pragma once
/**
 * Buddy & slab allocators
 */

#include "mm/frame.h"
#include "mm/startup.h"
#include "mm/vm.h"

#include "list.h"
#include <stdint.h>

#define SLAB_NUM_SLAB_SIZES 6

#define BUDDY_MAX_EXPONENT 18 // 1GB
// #define BUDDY_MAX_EXPONENT 10
// #define BUDDY_MAX_EXPONENT 5

#define BUDDY_NUM_FREE_LISTS (BUDDY_MAX_EXPONENT + 1)
#define MEMORY_START KVA_START

// SlabAllocator
//    manage slabs with the same allocation size,
//    A slab is a frame allocated with small objects with same size
//      + @SLAB_MAX_SLOTS: The number of pages available for a single slab
//        -> Frame is 4kb by architecture.
//          CortexA53 is 16 bytes aligned, so the here 16 bytes is set to be
//          the minimum size available for objects in slab.
//        -> Therefore, the maximum slots in slab is 4096/16 = 256
#define SLAB_MAX_SLOTS 256

//  The size of slab range from 16(2^4) to 512(2^9) bytes
#define SLAB_OBJ_MIN_SIZE_EXP 4
#define SLAB_OBJ_MAX_SIZE_EXP 9

typedef struct Frame {
  // inherit a list type, so we could cast FrameNode into list_head
  struct list_head list_base;

  // only if this page is slab allocated if this field to be useful
  uint8_t slot_available[SLAB_MAX_SLOTS];
  struct SlabAllocator *slab_allocator;
  int free_slot_remains;

  int arr_index;
  void *addr;
  int exp;
} Frame;

// BuddyAllocater
//    allocate contiguous frames
typedef struct BuddyAllocater {
  list_head_t free_lists[BUDDY_NUM_FREE_LISTS];
  struct Frame *frames;
} BuddyAllocater;

typedef struct SlabAllocator {
  int unit_size; // size of the unit (in Bytes)
  int max_slab_num_obj;

  BuddyAllocater *frame_allocator;

  struct Frame *cur_frame;  // The current frame be used for obejct allocation
  list_head_t partial_list; // Frames in full_list would be move to
                            //  partial_list once an object is freed
  list_head_t full_list;    // Once cur_frame is full, it would be dispatch to
                            //  full_list for storage.
} SlabAllocator;

// Call slab allocator for allocate an object
void *slab_alloc(SlabAllocator *alloc);

// Free an object
void slab_free(void *obj);

void buddy_init(BuddyAllocater *alloc, StartupAllocator_t *sa,
                struct Frame *frames);
struct Frame *buddy_alloc(BuddyAllocater *alloc, int size_in_byte);
void buddy_free(BuddyAllocater *alloc, struct Frame *frame);
void buddy_dump(BuddyAllocater *alloc);
