#include "gba_funcs.h"
#include "sav_debug_log.h"
#include <stdio.h>
#include <stdarg.h>

static size_t position = 0xFFFFFFFFUL;

__printflike(1,2) _Bool debug_log_printf(const char *restrict fmt, ...) {
  if (0UL==(~position))
    return false;
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char buf[len+1];
  va_start(args, fmt);
  vsnprintf(buf, len+1, fmt, args);
  if (buf[len]) {
    buf[len] = '\n';
  }

  va_end(args);

  if (!SRAM_Write(buf, sizeof(buf), position)) {
    static const u64 l = 0ULL;
    size_t rem;
    for (size_t i = 0; i < sizeof(buf); i+=sizeof(l)) {
      SRAM_Write(&l, sizeof(l), position+i);
    }
    if ((rem = sizeof(buf)%sizeof(l))!=0) {
      size_t ofs = position + sizeof(buf) - rem;
      for (size_t i = 0; i < rem; ++i)
        SRAM_Write(&l, 1, ofs+i);
    }
    return false;
  }

  position+=sizeof(buf);

  return true;
}

void debug_log_initialize(void) {
  position = 0UL;
  static const u64 l = 0ULL;
  for (size_t i = 0; i < 0x00010000UL; i+=sizeof(l)) {
    SRAM_Write(&l, sizeof(l), i);
  }


}
