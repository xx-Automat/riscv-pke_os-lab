#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/spike_file.h"
#include "util/string.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void get_filename(char *filepath, addr_line *line) {
  code_file *cf = current->file + line->file;
  char *dir=(current->dir)[cf->dir];
  char *filename = cf->file;
  strcpy(filepath, dir);
  // filepath = dir + "/" + filename
  filepath += strlen(dir);
  *filepath++ = '/';
  strcpy(filepath, filename);
}

static void print_errorline(char *filename, uint64 line) {
  sprint("Runtime error at %s:%ld\n", filename, line);
  char filebuf[800];
  spike_file_t *file = spike_file_open(filename, O_RDONLY, 0);
  if (IS_ERR_VALUE(file)) panic("Fail on openning the error source file.\n");
  spike_file_pread(file, (void *)filebuf, sizeof(filebuf), 0);
  char *st = filebuf;
  line--; // start from 0
  // traverse by line
  while (line--) {
    while (*st++ != '\n');  // move the pointer in one line
  } // have found the errorline
  char *ed = st;
  while (*ed++ != '\n');  // move the pointer in the errorline
  *ed = '\0';
  sprint(st);
  spike_file_close(file);
}

static void handle_illegal_instruction(uint64 epc) { 
  addr_line *al = current->line;
  int i;
  for (i = 0; i < current->line_ind; ++i, ++al) {
    if (al->addr == epc) break;
  }
  if (i == current->line_ind) panic("Illegal instruction handle error!");
  char filename[100];
  get_filename(filename, al);
  print_errorline(filename, al->line);
  panic("Illegal instruction!"); 
}

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() { panic("Misaligned AMO!"); }

static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//
// handle_mtrap calls cooresponding functions to handle an exception of a given type.
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
      //panic( "call handle_illegal_instruction to accomplish illegal instruction interception for lab1_2.\n" );
      handle_illegal_instruction(read_csr(mepc));
      break;
    case CAUSE_MISALIGNED_LOAD:
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}

