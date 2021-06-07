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

// With a 4K granule, we expect at most 4-level page table
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
 *  (Referring to the VMSAv8-32 Long-descriptor format)
 *  may be marked as:
 *  1. Block descriptor: gives the base addr of
 *    a block of memory, also mark this entry as an
 *    end of translatoin lookup
 *  2. Table descriptor: gives the base addr of the
 *    next level of translation table
 *  3. Invalid
 *
 *  Descriptor Format (Long-descriptor format)
 *  Block/Table entries: (upper_attr, output_addr, lower_attr, pd_type)
 */
// PD type
#define PD_TYPE_TABLE 0b11
#define PD_TYPE_BLOCK 0b01
#define PD_TYPE_INVALID 0b00

// PD_ATTR
#define PD_ATTR_ACCESS (1 << 10)
#define PD_ATTR_MAIR(MAIR_IDX) (MAIR_IDX << 2)

// Base schema for page descriptors

// Table descriptor
#define BOOT_PD_TABLE (PD_TYPE_TABLE)
// Block descriptor for device memory
#define BOOT_PD_BLOCK_DEV                                                      \
  (PD_ATTR_ACCESS | PD_ATTR_MAIR(MAIR_IDX_DEVICE_nGnRnE) | PD_TYPE_BLOCK)
// Block descriptor for normal memory
#define BOOT_PD_BLOCK_NORMAL                                                   \
  (PD_ATTR_ACCESS | PD_ATTR_MAIR(MAIR_NORMAL_NOCACHE) | PD_TYPE_BLOCK)

/**
 * Boot Page table config
 * Identity paging for bare metal system
 */

// Each entry is 8 Bytes (64bit), so we need 0x1000 Bytes for each table
#define BOOT_ADDR_L0_TABLE 0x0
#define BOOT_ADDR_L1_TABLE 0x1000
#define BOOT_ADDR_L2_TABLE 0x2000

#define L0_ENTRY (BOOT_ADDR_L1_TABLE | BOOT_PD_TABLE)

// Each entry in L1 control the mapping with size=1G
// Because we only use the first 2G address, so we only need to setup
//  identity paging for the first 2G.
// entry0 (0-1G): Forward to BOOT_L2_TABLE
//    note: address from 0 - 1G [0x0 - 0x4000_0000)
#define L1_ENTRY_SUB_TABLE (BOOT_ADDR_L2_TABLE | BOOT_PD_TABLE)
// entry1 (1-2G): to represent a block of device memory (ARM Pheripherals)
//    note: address from 1 - 2G [0x4000_0000 - 0x8000_0000]
#define L1_ENTRY_ARM_PERIF (0x40000000 | BOOT_PD_BLOCK_DEV)

// Addrees from 3F00_0000 to 3FFF_FFFF is used for Pheripherals
#define LOWER_1G_PERIF_START 0x3F000000

#ifndef __ASSEMBLER__
// Only use for C files

#endif