.section ".text.relocate"

.global _relocate

_relocate:
    // Get cpuid
    mrs x0, MPIDR_EL1
    and x0, x0, #3 // Get the first two bit in Aff0 fields
    cbz x0, 2f     // only cpuid=0 could go to label 2

// Busy Loop
1:
    wfe
    b 1b

// cpuid = 0
2:
    // Initialize stack pointer
    ldr x0, = __relocate_target_addr
    mov sp, x0

    // Clear bss
    ldr     x0, =__bss_start
    ldr     x1, =__bss_size
3:  cbz     x1, 4f          // If size(bss)=0, bss is cleared
    str     xzr, [x0], #8   //  *x0 = zero, x0 = x0 + 8
    sub     x1, x1, #1      //  x1 = x1 - 1
    cbnz    x1, 3b          // Loop back to label 3 until bss is cleared
4:  bl      relocate


//This section would be executed right after relocation
.section ".text.boot"

.global _start

_start:
1:  bl      main  // Jump to Kernel Entry Point (Main function)
    b       1b    // Stay in busy loop if returned
