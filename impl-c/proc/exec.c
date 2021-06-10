#include "proc/exec.h"
#include "proc/argv.h"
#include "proc/task.h"

// #include "dev/cpio.h"
#include "fatal.h"
#include "fs/vfs.h"
#include "memory.h"
#include "mm/frame.h"
#include "mm/mmu.h"
#include "mm/vm.h"
#include "stddef.h"
#include "stdint.h"
#include "timer.h"
#include "uart.h"

#include "config.h"
#include "log.h"

#ifdef CFG_LOG_PROC_EXEC
static const int _DO_LOG = 1;
#else
static const int _DO_LOG = 0;
#endif

void exec_user(const char *name, char *const argv[]) {
  struct task_struct *task = get_current();
  log_println("[exec] name:%s cur_task: %d(%x)", name, task->id, task);
  // TODO: may need to free old page table
  task->page_table = new_empty_page_table();
  log_println("[exec] new pgd allocated: %x", task->page_table);

  // load new program into memory, also setup page table
  struct file *f = vfs_open(name, 0);
  if (f == NULL) {
    uart_println("[Loader]Cannot found `%s` in vfs", name);
    FATAL("Should check file exists before running loader");
  }
  uintptr_t page_kva = (uintptr_t)map_new_addr(
      task->page_table, DEFAULT_USPACE_LOAD_ADDR, MAP_TYPE_USER_CODE);
  int total_pages = 1;
  for (;; page_kva += FRAME_SIZE, total_pages += 1) {
    // TODO: support code larger than 1 frame
    int cnt = vfs_read(f, (void *)page_kva, FRAME_SIZE);
    uart_println("num bytes load... :%d", cnt);
    if (cnt < FRAME_SIZE) {
      break;
    }
  }
  vfs_close(f);
  log_println("[exec] load new program, total pages: %d", total_pages);

  // allocate a new user stack to use. Initialy it's 4K

  uintptr_t page_start = (uintptr_t)map_new_addr(
      task->page_table, USPACE_MEM_TOP - FRAME_SIZE, MAP_TYPE_USER_MEM);
  uintptr_t page_end = page_start + FRAME_SIZE;

  // place args onto the newly allocated user stack
  uintptr_t new_sp;
  place_args(page_end, argv, NULL, NULL, &new_sp);

  uintptr_t page_offset = page_end - new_sp;
  uintptr_t user_sp = USPACE_MEM_TOP - page_offset;

  // Access to low memory area equals to access user space now
  set_pgd_low((uintptr_t)task->page_table);

  uart_println("access user argc");
  int argc = *(int *)user_sp;
  char **argv0 = (char **)(user_sp + 8);

  uart_println("get-> %d(%x)", argc, user_sp);
  uart_println("argv0: %s", argv0[0]);
  // FATAL("STOP here");

  // TODO: handle sys_exec, may need to free up old code frame
  // Caller of this function is either
  //  1. a kernel thread with a user thread called sys_exec
  // or
  //  2. a kernel thread spawning a new user thread

  // task->user_stack = user_stack;
  // task->user_sp = new_sp;

  // context under kernel mode
  task->cpu_context.fp = task->kernel_stack + FRAME_SIZE;
  task->cpu_context.sp = task->kernel_stack + FRAME_SIZE;

  // Jump into user mode
  asm volatile("mov x0, 0x340  \n"); // enable core timer interrupt
  asm volatile("msr spsr_el1, x0  \n");
  asm volatile("msr sp_el0, %0    \n" ::"r"(user_sp));
  asm volatile("msr elr_el1, %0   \n" ::"r"(DEFAULT_USPACE_LOAD_ADDR));

  asm volatile("mov x0, %0\n\
                mov x1, %1 \n\
                eret" ::"r"(user_sp),
               "r"(argv0));
}