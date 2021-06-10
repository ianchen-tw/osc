#include "mm/vm.h"
#include "mm/mmu.h"

#include "fatal.h"
#include "memory.h"
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "log.h"

#ifdef CFG_LOG_VM
static const int _DO_LOG = 1;
#else
static const int _DO_LOG = 0;
#endif

#define PAGE_TABLE_SIZE (8 * 512)

#define MASK_9Bits 0x1ff

#define PGD_IDX(addr) (((addr) >> 39) & MASK_9Bits)
#define PUD_IDX(addr) (((addr) >> 30) & MASK_9Bits)
#define PMD_IDX(addr) (((addr) >> 21) & MASK_9Bits)
#define PTE_IDX(addr) (((addr) >> 12) & MASK_9Bits)

// addr: dp[39:12]
#define DP_ADDR_MASK 0xFFFFFFF000
#define DP_ADDR(dp) (dp & DP_ADDR_MASK)

// Page descriptor for table entry
#define make_descriptor(addr, attr) ((addr) | (attr))

// Return the kva of a newly allocate page table
void *new_empty_page_table() {
  // allocator directly output physical address
  int64_t *table = (int64_t *)kalloc(PAGE_TABLE_SIZE);
  for (int i = 0; i < 512; i++) {
    table[i] = PD_TYPE_INVALID;
  }
  return (void *)table;
}

// Return the index of each table
void get_page_idx(uintptr_t va, int *pgd_idx, int *pud_idx, int *pmd_idx,
                  int *pte_idx) {
  if (pgd_idx != NULL) {
    *pgd_idx = PGD_IDX(va);
  }
  if (pud_idx != NULL) {
    *pud_idx = PUD_IDX(va);
  }
  if (pmd_idx != NULL) {
    *pmd_idx = PMD_IDX(va);
  }
  if (pte_idx != NULL) {
    *pte_idx = PTE_IDX(va);
  }
  return;
}

void set_pgd_low(uint64_t next_pgd) {
  asm volatile("\
                mov x0, %0\n\
                dsb ish\n\
                msr ttbr0_el1, x0\n\
                tlbi vmalle1is\n\
                dsb ish\n\
                isb" ::"r"(next_pgd));
}

void *map_new_addr(void *pgd_addr, uintptr_t va, int map_type) {
  int entry_idx[4];
  get_page_idx(va, &entry_idx[0], &entry_idx[1], &entry_idx[2], &entry_idx[3]);
  log_println("[VM] map new page to pgd:%x with va:%x", pgd_addr, va);

  // Create descriptor tables along page walk
  //    we can't use pa directly becuase using this low addrees area
  //    would result in using client virtual address
  int64_t *cur_table = (int64_t *)pa_to_kva(pgd_addr);
  int idx;
  for (int i = 0; i < 3; i++) {
    uintptr_t new_pt;
    int64_t *next_table;
    idx = entry_idx[i];

    // required entry not exists
    if (PD_TYPE_INVALID == (cur_table[idx] & MASK_PD_TYPE)) {
      new_pt = (uintptr_t)new_empty_page_table();
      cur_table[idx] = make_descriptor(kva_to_pa(new_pt), PD_TYPE_TABLE);
      next_table = (int64_t *)new_pt;
    } else {
      next_table = (int64_t *)pa_to_kva(DP_ADDR(cur_table[idx]));
    }
    cur_table = next_table;
  }
  int64_t *pte = cur_table;
  // alloc a new page
  uintptr_t page_addr = (uintptr_t)kalloc(PAGE_TABLE_SIZE);
  uint64_t page_attr;

  if (map_type == MAP_TYPE_USER_CODE) {
    page_attr = USER_ATTR_CODE;
  } else if (map_type == MAP_TYPE_USER_MEM) {
    page_attr = USER_ATTR_STACK;
  } else {
    uart_println("unknown map type: %d", map_type);
    FATAL("Kernel bug");
  }
  pte[entry_idx[3]] = make_descriptor(kva_to_pa(page_addr), page_attr);
  return (void *)page_addr;
}