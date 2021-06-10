#pragma once

/**
 * Memory.h
 * Provide interface for dynamic memory allocation
 */

// Allocate a block of memory to use in kernel space (KVA)
void *kalloc(int size);

// Free a memory block (KVA)
void kfree(void *addr);

// Initialize dynamic memory allocator
void KAllocManager_init();
void KAllocManager_show_status();

// Run several allocation/free as an example
void KAllocManager_run_example();