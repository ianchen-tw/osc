#include "memory.h"
#include "string.h"
#include "uart.h"

#include "config.h"
#include "fatal.h"
#include "log.h"
#include "test.h"

#include <stdint.h>

#ifdef CFG_LOG_PROC_ARGV
static const int _DO_LOG = 1;
#else
static const int _DO_LOG = 0;
#endif

// sp value must be 16 bytes aligned
#define SP_ALIGN 16

static inline uintptr_t align_up(uintptr_t n, unsigned long align) {
  return (n + align - 1) & (~(align - 1));
}

/**
 * @brief Place argv into user stack
 * @param src_sp the original user sp value
 * @param src_argv argv to copy from
 * @param ret_argc  address of the argc
 * @param ret_argv0 address of the argv0
 * @param ret_sp new sp value
 */
void place_args(/*IN*/ uintptr_t src_sp,
                /*IN*/ const char **src_argv,
                /*OUT*/ uintptr_t *ret_argc,
                /*OUT*/ uintptr_t *ret_argv0,
                /*OUT*/ uintptr_t *ret_sp) {

  log_println("[place_arg] src_sp:%x", src_sp);
  if ((src_sp % SP_ALIGN) != 0) {
    uart_println("[place arg] src_sp must be %d bytes aligned, get:%x",
                 SP_ALIGN, src_sp);
    FATAL("Check sp before calling this function");
  }

  int nm_args = 0;
  while (src_argv[nm_args] != NULL) {
    log_println(" src_argv[%d](%x):`%s`", nm_args, src_argv[nm_args],
                src_argv[nm_args]);
    nm_args++;
  }

  // Save Arguments into kernel memory.
  // We need to backup argv since they are stored in the user stack,
  //  which would be overwritten by following operations.
  char **saved_args = (char **)kalloc(sizeof(char *) * nm_args);
  char *arg_str = NULL;

  for (int i = 0; i < nm_args; i++) {
    arg_str = (char *)kalloc(sizeof(char) * (strlen(src_argv[i]) + 1));
    strcpy(arg_str, src_argv[i]);
    saved_args[i] = arg_str;
  }

  /**
   * Save argv into user stack.
   *
   * High
   * |         | / old stack top (16 bytes aligned )
   * | "str2"  | <- `args_offset[2]` (16 bytes aligned )
   * |         |
   * | "str1"  | <- `args_offset[1]` (16 bytes aligned )
   * |         |
   * | "str0"  | <- `args_offset[0]` (16 bytes aligned )
   * | argv[2] |
   * | argv[1] |
   * | argv[0] |
   * | argc    | <- new stack top (16 bytes aligned )
   * Low
   */

  uintptr_t sp = src_sp;
  size_t _size = 0;
  uintptr_t *addrs = kalloc(sizeof(char *) * nm_args);
  log_println("[place_arg] start copy, sp: %x", sp);

  // copy args
  for (int i = nm_args - 1; i >= 0; i--) {
    _size = (sizeof(char) * strlen(saved_args[i])) + 1;
    _size = align_up(_size, SP_ALIGN);
    sp -= _size;
    log_println("[place_arg] size for src_argv[%d]: %d(byte), sp after:%x", i,
                _size, sp);
    addrs[i] = sp;
    strcpy((char *)(sp), saved_args[i]);
  }

  // Start of the argv array is 16 byte aligned
  // we put the argv from back
  int size_argv = ((sizeof(char **)) * nm_args);
  int size_argc = sizeof(size_t);
  int total_size = size_argv + size_argc;
  if (total_size < align_up(total_size, 16)) {
    int back_indent = align_up(total_size, 16) - total_size;
    sp -= back_indent;
  }

  // fillup argv array
  for (int i = nm_args - 1; i >= 0; i--) {
    _size = (sizeof(char **));
    sp -= _size;
    log_println("[place_arg] store pointer to argv[%d]: %d(byte), sp after:%x",
                i, _size, sp);
    // argv: pointer to char*
    *(char **)sp = (char *)addrs[i];
  }
  if (ret_argv0 != NULL) {
    *ret_argv0 = sp;
  }
  kfree(addrs);

  // place argc
  _size = (sizeof(size_t));
  sp -= _size;
  log_println("[place_arg] store argc: %d(byte), sp after:%x", _size, sp);
  *((size_t *)sp) = nm_args;
  if (ret_argc != NULL) {
    *ret_argc = sp;
  }

  if (ret_sp != NULL) {
    *ret_sp = sp;
  }
  // Caution: need to be freed.
  for (int i = 0; i < nm_args; i++) {
    kfree(saved_args[i]);
  }
  kfree(saved_args);
}

#ifdef CFG_RUN_PROC_ARGV_TEST
bool test_good() {
  char *src_argv[] = {"hello_world.out", "--name", "ian", NULL};
  char stack[400];

  uintptr_t src_sp = align_up((uintptr_t)&stack[350], SP_ALIGN);

  uintptr_t new_sp;
  uintptr_t addr_argv0;
  uintptr_t addr_argc;

  place_args(src_sp, (const char **)src_argv, &addr_argc, &addr_argv0, &new_sp);

  char **new_argv = (char **)addr_argv0;
  int argc = *(int *)addr_argc;

  // uart_println("returned sp: %x", new_sp);
  // uart_println("returned argc: %d(%x)", argc, addr_argc);
  // for (int i = 0; i < argc; i++) {
  //   uart_println("returned argv[%d]: (%x)", i, &new_argv[i]);
  // }
  if ((new_sp % SP_ALIGN) != 0) {
    uart_println("Returned sp is not 16 byte aligned: %d", new_sp);
    return false;
  }
  if (new_sp >= src_sp) {
    uart_println("returned sp higher than given");
    return false;
  }
  if (argc != 3) {
    uart_println("Wrong argc :%d, expect:%d", argc, 3);
    return false;
  }

  for (size_t i = 0; i < argc; i++) {
    if (new_sp >= (uintptr_t)new_argv[i]) {
      uart_println("argv[%d] (%x) is higher than new_sp:%x", i, &new_argv[i],
                   new_sp);
      return false;
    }
    if (0 != strcmp(new_argv[i], src_argv[i])) {
      uart_println("wrong argv[%d]: %s, expect:%s", i, new_argv[i],
                   src_argv[i]);
      return false;
    }
  }
  return true;
}
#endif

void test_argv_parse() {
#ifdef CFG_RUN_PROC_ARGV_TEST
  unittest(test_good, "proc", "argv");
#endif
}