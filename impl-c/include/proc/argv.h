#pragma once

#include <stdint.h>

/**
 * @brief Place argv into user stack
 * @param src_sp the original user sp value
 * @param src_argv argv to copy from
 * @param ret_argc (total argc count)
 * @param ret_argv (alt)
 * @param ret_sp new sp value
 */
void place_args(/*IN*/ uintptr_t src_sp,
                /*IN*/ char *const src_argv[],
                /*OUT*/ int *ret_argc,
                /*OUT*/ char ***ret_argv,
                /*OUT*/ uintptr_t *ret_sp);

// Only used for running tests
void test_argv_parse();