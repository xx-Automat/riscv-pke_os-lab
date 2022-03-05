#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/spike_file.h"
#include "util/string.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void get_filename(char *name, addr_line *line){
  code_file *file = current->file + line->file;
  char *dir=(current->dir)[file->dir];
  char *filename = file->file;
  strcpy(name, dir);
  name += strlen(dir);
  *name++ = '/';
  strcpy(name, filename);
}

static void print_file_line(char *filename,uint64 linenum){
  sprint("Runtime error at %s:%lld\n",filename,linenum);
  char filebuf[800];
  spike_file_t *file=spike_file_open(filename, O_RDONLY, 0);
  if (IS_ERR_VALUE(file)) panic("Fail on openning the error source file.\n");
  spike_file_pread(file, (void *)filebuf, 800, 0);
  // if(spike_file_pread(file,(void *)filebuf,800,0)!=800) panic("Fail on loading error source file.\n");
  char *p = filebuf;
  linenum--;
  while (linenum--) { //skip one '\n'
    while (*p++ != '\n');
  }
  char *end = p;
  while(*end++ != '\n');
  *end = 0;
  sprint(p);
  spike_file_close(file);
}

static void handle_illegal_instruction(uint64 epc) { 
  addr_line *line = current->line;
  int i;
  for (i = 0; i < current->line_ind; ++i, ++line) {
    if (line->addr == epc) break;
  }
  if (i == current->line_ind) panic("Illegal instruction handle error!");
  char filename[100];
  get_filename(filename, line);
  print_file_line(filename, line->line);
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

