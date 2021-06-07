#include "vm/mmu.h"

#define ADDR_L0_TABLE 0x0
#define ADDR_L1_TABLE 0x1000

#define L0_ENTRY (ADDR_L1_TABLE | BOOT_PGD_ATTR)

#define L1_ENTRY_RAM_GPU_PERIF (0x00000000 | BOOT_PUD_ATTR)
#define L1_ENTRY_ARM_PERIF (0x40000000 | BOOT_PUD_ATTR)


.global mmu_init
// We haven't setup stack, so be careful with stack memory
mmu_init:
    // Setup TCR: Translation Control Register for base config
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    // Setup MAIR: Memory Attribute Indirection Register for accessibility & cache
    // policies
    ldr x0, =MAIR_CONFIG_DEFAULT
    msr mair_el1, x0

    /**
    * Init Page Table, requires at least two table for enabling MMU
    * Here we hard coded the table content
    */
    // L0 table with only one entry to the L1 table
    ldr x0, =ADDR_L0_TABLE
    ldr x1, =L0_ENTRY
    str x1, [x0]

    // L1 table with two entries
    ldr x0, =ADDR_L1_TABLE
    ldr x1, =L1_ENTRY_RAM_GPU_PERIF
    str x1, [x0]
    ldr x2, =L1_ENTRY_ARM_PERIF
    str x2, [x0, 8]

    // Point to our page table
    // Use the same page table for both lower/higher VA region
    ldr x0, =ADDR_L0_TABLE
    msr ttbr0_el1, x0
    msr ttbr1_el1, x0
    isb

    // Enable MMU
    ldr x0, =STCLR_CONFIG_MMU_ENABLE
    mrs x2, sctlr_el1
    orr x2, x2, x0
    msr sctlr_el1, x2
    isb

    // The entire kernel code is linked to the higher memory region
    // However, the bootloader would load our kernel at 0x80000
    // After enabling the MMU, we're officially running in the
    // Address that are supposed to be linked to
    // mmu_init_finished: defined in entry.S
    b mmu_init_finished


