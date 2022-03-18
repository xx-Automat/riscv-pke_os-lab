/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe *, uint64 satp);

// current points to the currently running user-mode application.
process* current = NULL;

// start virtual address of our simple heap.
uint64 g_ufree_page = USER_FREE_ADDRESS_START;

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  write_csr(stvec, (uint64)smode_trap_vector);
  // set up trapframe values that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;      // process's kernel stack
  proc->trapframe->kernel_satp = read_csr(satp);  // kernel page table
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // set up the registers that strap_vector.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  write_csr(sstatus, x);

  // set S Exception Program Counter to the saved user pc.
  write_csr(sepc, proc->trapframe->epc);

  //make user page table
  uint64 user_satp = MAKE_SATP(proc->pagetable);

  // switch to user mode with sret.
  return_to_user(proc->trapframe, user_satp);
}

uint64 better_malloc(int n) {
  n = ROUNDUP(n, 8);
  if (n > PGSIZE) panic("The malloc size n should be smaller than PGSIZE.");
  // if the free block queue is empty
  if ((block *)current->free_head == NULL) {
    // allocate a new page
    void* pa = alloc_page();
    uint64 va = g_ufree_page;
    g_ufree_page += PGSIZE;
    user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
    current->free_head = (block *)pa;
    current->free_head->size = PGSIZE - sizeof(block);
    current->free_head->next = NULL;
    current->free_head->va = va;
    current->free_head->used = 0;
  }
  // free block queue is not empty
  block *cur = current->free_head;
  // browse the queue to see if there is already a block big enough (FF like???)
  while (cur->size < n && cur->next) {
    cur = cur->next;
  }
  // no existing block big enough is found, malloc a new block 
  if (cur->size < n) { // cur->next = NULL
    void* pa = alloc_page();
    uint64 va = g_ufree_page;
    g_ufree_page += PGSIZE;
    user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
    block *tb = (block*)pa;
    tb->size = PGSIZE - sizeof(block);
    tb->next = NULL;
    tb->va = va;
    tb->used = 0;
    cur->next = tb;
    cur = tb;
  }
  uint64 res_va = cur->va + sizeof(block);
  uint64 res_pa = (uint64)cur + sizeof(block);
  // After allocation, there is enough space left to divide into new blocks
  if (cur->size - n > sizeof(block)) {
    block *nb = (block*)(res_pa + n);
    nb->used = 0;
    nb->va = res_va + n;
    nb->size = cur->size - n - sizeof(block);
    nb->next = cur->next;
    if (cur == current->free_head)
      current->free_head = nb;
  } else {
    if (cur == current->free_head)
      current->free_head = cur->next;
  }
  cur->size = n;
  cur->used = 1;
  cur->next = current->used_head;
  current->used_head = cur;
  return res_va;
}

void better_free(uint64 va) {
  block *tb = current->used_head;
  while (tb) {
    if (va >= tb->va && va <= tb->va + tb->size)
      break;
    tb = tb->next;
  }
  if (tb == current->used_head) { 
    current->used_head = tb->next;
  }
  tb->used = 0;
  tb->next = current->free_head;
  current->free_head = tb;
}