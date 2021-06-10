#pragma once
/**
 * Linear mapping
 *  We our kernel is linked to higher memory area(KVA), which is only a offset
 * of actual phisical address.
 */

// VA: virutal address
// PA: physical address

#define kva_to_pa(addr) (0x0000ffffffffffff & (uint64_t)addr)
#define pa_to_kva(addr) (0xffff000000000000 | (uint64_t)addr)

#define KVA_START 0xffff000000000000

#ifndef __ASSEMBLER__
// Only inlcude for C files
#include <stdint.h>
void *new_empty_page_table();

/**
 * @brief Allocate a new page in givin va and map it to user's page table
 * @param pgd_addr address of the page global directory(pgd)
 * @param va virtual address in the address space mapped by pgd
 * @retval Address of the newly allocated page (in KVA)
 */
void *map_new_addr(void *pgd_addr, uintptr_t va, int map_type);

// Set page table for low mem area
void set_pgd_low(uint64_t next_pgd);

#endif