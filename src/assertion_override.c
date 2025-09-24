/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/

#include "gba_types.h"
#ifdef _DEBUG_BUILD_
#define EXIT_PROCEDURE() do continue; while (1)
#else
#include <stdlib.h>
#define EXIT_PROCEDURE() exit(EXIT_FAILURE)
#endif
#include "mode3_io.h"
void __assert_func(const char *src_filename, int line_no, const char *caller, const char *assertion) {
  (*((volatile u16 *)(0x04000000 + 0x0208))) = 0;
  (*((IRQ_Callback_t *)(0x03007FFC))) = ((void *)0);
  (*((volatile u16 *)(0x04000000 + 0x0200))) ^= 1;
  mode3_printf(0, 0, 0x1069,
               "[Error]:\x1b[0x10A5] Assertion expression, \x1b[0x2483]"
               "%s"
               "\x1b[0x10A5], has \x1b[0x1069]failed.\x1b[0x8000]\n\t"
               "\x1b[0x2483]Assertion from Function: \x1b[0x7089]%s\x1b[0x8000]\n\t"
               "\x1b[0x2483]Defined In: \x1b[0x7089]%s\x1b[0x8000]\t\x1b[0x2483]Line No.: \x1b[0x7089]%d\n"
               , assertion, caller, src_filename, line_no);
  EXIT_PROCEDURE();
}

