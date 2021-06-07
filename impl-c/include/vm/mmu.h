// D7.2 Gernal system control registers

#pragma once
/**
 * TCR: Translation Control Register, which controls
 *  1. (IPS) Intermediate Physical Address Size (32/36/40/42/44/48 bits)
 *  2. (TG) Translation Granule: The page size manage by MMU (4/64K)
 *  3. (SZ) Number of the most significant bits that must be either all 0s or
 *        all 1s, or.. THe size offset of the memory region address by TTBRx_ELx
 */
#define TCR_OFFSET_T1SZ 16
#define TCR_OFFSET_T0SZ 0
#define TCR_TxSZ_ADDRESS_48bit (64 - 48)
#define TCR_CONFIG_REGION_48bit                                                \
  ((TCR_TxSZ_ADDRESS_48bit << TCR_OFFSET_T0SZ) |                               \
   (TCR_TxSZ_ADDRESS_48bit << TCR_OFFSET_T1SZ))

#define TCR_TG0_4KB (0b00 << 14)
#define TCR_TG1_4KB (0b10 << 30)
#define TCR_CONFIG_4KB (TCR_TG0_4KB | TCR_TG1_4KB)
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/**
 * MAIR: Memory Attribute Indirection Register (Cache policies)
 * Provide memory attribute encodings
 * Device attributes:
 *  1. Gathering(G/nG): Merge multiple accesses into a bus transaction
 *  2. Reording(R/nR): Reorder the accesses to the same device
 *  3. Early Write Ack (E/nE): Ack response could come from buffer
 */
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1
// Peripheral access
#define MAIR_DEVICE_nGnRnE 0b00000000
// Normal RAM access
#define MAIR_NORMAL_NOCACHE 0b01000100

#define MAIR_CONFIG_DEFAULT                                                    \
  ((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) |                      \
   (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))

#define STCLR_OFFSET_ELx_MMU 0
#define STCLR_MMU_ENABLE 1
#define STCLR_CONFIG_MMU_ENABLE (STCLR_MMU_ENABLE << STCLR_OFFSET_ELx_MMU)

/**
 * Page descriptor: entries in the page table
 *  may be marked as:
 *  1. Entry to the next level page-table
 *  2. Block/Table entry:
 *  3. Invalid

 */
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_ACCESS (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
