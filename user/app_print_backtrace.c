/*
 * Below is the given application for lab1_challenge1_backtrace.
 * This app prints all functions before calling print_backtrace().
 */

#include "user_lib.h"
#include "util/types.h"

void f8() { print_backtrace(9); }
void f7() { f8(); }
void f6() { f7(); }
void f5() { f6(); }
void f4() { f5(); }
int f3() { f4(); return 0; }
void f2(int x) { f3(); }
void f1() { f2(1); }

int main(void) {
  printu("back trace the user app in the following:\n");
  f1();
  exit(0);
  return 0;
}
