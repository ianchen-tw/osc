#include "mm/mmu.h"
#include "mm/vm.h"

// Define a section near the kernel entrypoint makes it
// easier to debug..
.section ".text.mmu_init"

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
    ldr x0, =BOOT_ADDR_L0_TABLE
    ldr x1, =L0_ENTRY
    str x1, [x0]

    // L1 table with two entries
    ldr x0, =BOOT_ADDR_L1_TABLE
    ldr x1, =L1_ENTRY_SUB_TABLE
    str x1, [x0]
    ldr x2, =L1_ENTRY_ARM_PERIF
    str x2, [x0, 8]

    // L2 table
fill_first_1G_table:
    ldr x5, =BOOT_PD_BLOCK_DEV
    ldr x6, =BOOT_PD_BLOCK_NORMAL
    ldr x10,=LOWER_1G_PERIF_START
    ldr x0, =BOOT_ADDR_L2_TABLE
    mov x1, #0  // address of mapped memory
    mov x2, #512 // number of entries remains

    set_entry:
        cbz x2, fill_table_end
        cmp x1, x10
        blt set_entry_normal
        set_entry_device:
            orr x3, x1, x5
            b set_entry_end
        set_entry_normal:
            orr x3, x1, x6
            b set_entry_end
    set_entry_end:
        str x3,[x0]
        add x0, x0, #8 // each entry take 8 bytes space
        sub x2, x2, #1
        add x1, x1, #0x200000 //each entry specify 2M addres
        b set_entry
fill_table_end:

    // Point to our page table
    // Use the same page table for both lower/higher VA region
    ldr x0, =BOOT_ADDR_L0_TABLE
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
    ldr x0 ,=KVA_START
    add x30, x30, x0
    ret
