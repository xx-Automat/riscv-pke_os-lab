#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"

typedef struct trapframe {
  // space to store context (all common registers)
  /* offset:0   */ riscv_regs regs;

  // process's "user kernel" stack
  /* offset:248 */ uint64 kernel_sp;
  // pointer to smode_trap_handler
  /* offset:256 */ uint64 kernel_trap;
  // saved user process counter
  /* offset:264 */ uint64 epc;

  //kernel page table
  /* offset:272 */ uint64 kernel_satp;
}trapframe;

typedef struct mem_control_block {
  uint64 used;
  uint64 size;
  struct mem_control_block *next;
  uint64 va;
}block;

// the extremely simple definition of process, used for begining labs of PKE
typedef struct process {
  // pointing to the stack used in trap handling.
  uint64 kstack;
  // user page table
  pagetable_t pagetable;
  // trapframe storing the context of a (User mode) process.
  trapframe* trapframe;

  block *free_head;
  block *used_head;
}process;

// switch to run user app
void switch_to(process*);

// current running process
extern process* current;
// virtual address of our simple heap
extern uint64 g_ufree_page;

uint64 do_better_malloc(int n);
void do_better_free(uint64 va);
#endif
