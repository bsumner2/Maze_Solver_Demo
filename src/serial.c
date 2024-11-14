#include "gba_funcs.h"
#include "gba_mmap.h"
#include "gba_types.h"
#include "queue.h"



void SIO_Set_Multi(u16 baud) {
  SIO_Cnt_t reg = {.raw = REG_SIO_CNT};
  reg.multi.fill0 = 32;
  reg.multi.baud = baud&3;
  REG_SIO_RCNT &= 0x000F;
  REG_SIO_CNT = reg.raw;
  do {
    vsync();
  } while (!(reg=(SIO_Cnt_t){.raw=REG_SIO_CNT}).multi.SD);
}



