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

#endif