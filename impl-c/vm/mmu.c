#include "vm/mmu.h"

#define ADDR_L0_TABLE 0x0
#define ADDR_L1_TABLE 0x1000

void mmu_lower_init() {
  // Convinent to debug by aborting lower VA region
  asm volatile("msr ttbr0_el1, %0\n" ::"r"(-1));
}

void mmu_init() {
  // We haven't setup stack, so be careful with stack memory

  // Setup TCR: Translation Control Register for base config
  asm volatile("msr tcr_el1, %0\n" ::"r"(TCR_CONFIG_DEFAULT));
  // Setup MAIR: Memory Attribute Indirection Register for accessibility & cache
  // policies
  asm volatile("msr mair_el1, %0\n" ::"r"(MAIR_CONFIG_DEFAULT));

  /**
   * Init Page Table, requires at least two table for enabling MMU
   * Here we hard coded the table content
   */
  // L0 table with only one entry to the L1 table
  asm volatile("str %0, [%1]\n" ::"r"(ADDR_L1_TABLE | BOOT_PGD_ATTR),
               "r"(ADDR_L0_TABLE));
  // L1 table with two entries
  asm volatile("str %0, [%1]\n" ::"r"(0x00000000 | BOOT_PUD_ATTR),
               "r"(ADDR_L1_TABLE));
  asm volatile("str %0, [%1]\n" ::"r"(0x40000000 | BOOT_PUD_ATTR),
               "r"(ADDR_L1_TABLE + 8));
  // Use the same page table for both lower/higher VA region
  //  ISB - Forces these changes to be seen by the next instruction
  asm volatile("msr ttbr0_el1, %0\n" ::"r"(ADDR_L0_TABLE));
  asm volatile("msr ttbr1_el1, %0\n" ::"r"(ADDR_L0_TABLE));
  asm volatile("ISB\n");

  // Enable MMU
  asm volatile("\
    mrs x2, sctlr_el1\n\
    orr x2 , x2, %0\n\
    msr sctlr_el1, x2\n\
    ISB \n\
  " ::"r"(STCLR_CONFIG_MMU_ENABLE));

  // The entire kernel code is linked to the higher memory region
  // However, the bootloader would load our kernel at 0x80000
  // After enabling the MMU, we're officially running in the
  // Address that are supposed to be linked to
  asm volatile("\
    ldr x0, =0xffff000000000000\n\
    add x30, x30, x0\n\
  " ::);
}