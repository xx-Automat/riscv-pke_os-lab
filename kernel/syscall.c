/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"

#include "elf.h"

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// implement the SYS_user_backtrace syscall
//
ssize_t sys_user_backtrace(uint64 depth)
{
  uint64 *fp = (uint64 *)current->trapframe->regs.s0; // 810fffa0
  // sprint("/****** debug *****/  *fp=0x%lx\n", *fp); // 810fffc0
  for (int i = 0; i < depth; ++i)
  {
    uint64 ra = *(fp + 2 * i + 1);//810002d8 810002ec + 14 = 81000300 81000314 81000334
    // sprint("/****** debug *****/ fp=0x%lx\n",fp + 2 * i + 1);
    // sprint("/****** debug *****/ ra=0x%lx\n",ra);
    char *func = find_func(ra);
    assert(func);
    sprint("%s\n", func);
    if (strcmp(func, "main") == 0) break;
  }
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_backtrace:
      return sys_user_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
