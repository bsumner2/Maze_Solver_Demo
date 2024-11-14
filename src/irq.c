#include "gba_mmap.h"
#include "gba_types.h"
#include "gba_util_macros.h"
#include "gba_funcs.h"
#include <sys/_intsup.h>

IRQ_Callback_Entry_t _G_ISR_TABLE[II_MAX] = {0};
int _G_ISR_HEAP_LAST_IDX = 0;
static i16 _L_ISR_LOCATIONS[II_MAX] = {0};

#define REG(addr) (*((volatile u16*) (addr)))

static const struct s_IRQ_sender {
  u16 reg_ofs;
  u16 flag;
} ALIGN(4) _L_IRQ_SENDERS[II_MAX] = {
  { 0x0004, 0x0008 },     // REG_DISPSTAT,    DSTAT_VBL_IRQ
  { 0x0004, 0x0010 },     // REG_DISPSTAT,    DSTAT_VHB_IRQ
  { 0x0004, 0x0020 },     // REG_DISPSTAT,    DSTAT_VCT_IRQ
  { 0x0102, 0x0040 },     // REG_TM0CNT,      TM_IRQ
  { 0x0106, 0x0040 },     // REG_TM1CNT,      TM_IRQ
  { 0x010A, 0x0040 },     // REG_TM2CNT,      TM_IRQ
  { 0x010E, 0x0040 },     // REG_TM3CNT,      TM_IRQ
  { 0x0128, 0x4000 },     // REG_SCCNT_L      BIT(14) // not sure
  { 0x00BA, 0x4000 },     // REG_DMA0CNT_H,   DMA_IRQ>>16
  { 0x00C6, 0x4000 },     // REG_DMA1CNT_H,   DMA_IRQ>>16
  { 0x00D2, 0x4000 },     // REG_DMA2CNT_H,   DMA_IRQ>>16
  { 0x00DE, 0x4000 },     // REG_DMA3CNT_H,   DMA_IRQ>>16
  { 0x0132, 0x4000 },     // REG_KEYCNT,      KCNT_IRQ
  { 0x0000, 0x0000 },     // cart: none 
};


static enum IRQ_Idx _L_flag_to_type(u16 flag) {
  // stupid hack for stupid mistake when designing this overengineered priority
  // queue based IRQ callback switchboard interface...
  switch (flag) {
    case (1<<II_VBLANK): return II_VBLANK;
    case (1<<II_HBLANK): return II_HBLANK;
    case (1<<II_VCOUNT): return II_VCOUNT;
    case (1<<II_TIMER0): return II_TIMER0;
    case (1<<II_TIMER1): return II_TIMER1;
    case (1<<II_TIMER2): return II_TIMER2;
    case (1<<II_TIMER3): return II_TIMER3;
    case (1<<II_SERIAL): return II_SERIAL;
    case (1<<II_DMA0): return II_DMA0;
    case (1<<II_DMA1): return II_DMA1;
    case (1<<II_DMA2): return II_DMA2;
    case (1<<II_DMA3): return II_DMA3;
    case (1<<II_KEYPAD): return II_KEYPAD;
    case (1<<II_GAMEPAK): return II_GAMEPAK;
    default: return 0xFFFF;
  }
}

static int IRQ_Heap_Bubble_Down(int origin) {
  IRQ_Callback_Entry_t entry, *entp;
  int cidx;
  u16 bubbling_entry_type = _L_flag_to_type(_G_ISR_TABLE[origin].flag);
  while ((cidx=origin*2+1) < _G_ISR_HEAP_LAST_IDX) {
    if (cidx+1 < _G_ISR_HEAP_LAST_IDX) {
      if (_G_ISR_TABLE[cidx+1].priority > _G_ISR_TABLE[cidx].priority)
        ++cidx;
    }
    if (_G_ISR_TABLE[cidx].priority <= _G_ISR_TABLE[origin].priority) {
      break;
    }
    
    entry = *(entp = _G_ISR_TABLE + origin);
    *entp =  _G_ISR_TABLE[cidx];
    _G_ISR_TABLE[cidx] = entry;
    entry = *entp;
    _L_ISR_LOCATIONS[_L_flag_to_type(entry.flag)] = origin;
    origin = cidx;
  }
  _L_ISR_LOCATIONS[bubbling_entry_type] = origin;
  return origin;
  
}

static int IRQ_Heap_Bubble_Up(int origin) {
  IRQ_Callback_Entry_t ent, *entp;
  int pidx = (origin-1)/2;
  u16 bubbling_entry_type = _L_flag_to_type(_G_ISR_TABLE[origin].flag);
  while (origin > 0 && _G_ISR_TABLE[origin].priority > _G_ISR_TABLE[pidx].priority) {
    ent = *(entp = _G_ISR_TABLE+origin);
    *entp = _G_ISR_TABLE[pidx];
    _G_ISR_TABLE[pidx] = ent;
    ent = *entp;
    _L_ISR_LOCATIONS[_L_flag_to_type(ent.flag)] = origin;
    origin = pidx;
    pidx = (origin-1)/2;
  }
  _L_ISR_LOCATIONS[bubbling_entry_type] = origin;
  return origin;
}


static int IRQ_Heap_Insert(enum IRQ_Idx interrupt_type, IRQ_Callback_t isr, u32 priority) {
  if (_G_ISR_HEAP_LAST_IDX >= II_MAX)
    return -1;
  _G_ISR_TABLE[_G_ISR_HEAP_LAST_IDX] = (IRQ_Callback_Entry_t){.priority = priority, .cb = isr, .flag = 1<<interrupt_type};

  if (!_G_ISR_HEAP_LAST_IDX) {
    return _G_ISR_HEAP_LAST_IDX++;
  }
  return IRQ_Heap_Bubble_Up(_G_ISR_HEAP_LAST_IDX++);
  
}

void IRQ_Init(IRQ_Callback_t isr_handler) {
  REG_IME = 0;
  fast_memset32(_L_ISR_LOCATIONS, 0xFFFFFFFF, sizeof(_L_ISR_LOCATIONS)/4);

  REG_ISR_MAIN = isr_handler ? isr_handler : ISR_MAIN_HANDLER;
  REG_IME = 1;

}


void IRQ_Enable(enum IRQ_Idx interrupt_type) {
  u16 tmp = REG_IME;
  REG_IME = 0;
  const struct s_IRQ_sender *sender = &_L_IRQ_SENDERS[interrupt_type];
  REG(REG_BASE + sender->reg_ofs) |= sender->flag;
  REG_IE |= 1<<interrupt_type;
  REG_IME = tmp;
}

void IRQ_Disable(enum IRQ_Idx interrupt_type) {
  u16 tmp = REG_IME;
  REG_IME = 0;
  const struct s_IRQ_sender *sender = &_L_IRQ_SENDERS[interrupt_type];
  REG(REG_BASE+sender->reg_ofs) ^= sender->flag;
  REG_IE ^= 1<<interrupt_type;
  REG_IME = tmp;
}

IRQ_Callback_t IRQ_Add(enum IRQ_Idx interrupt_type, IRQ_Callback_t isr, u32 priority) {
  IRQ_Callback_t ret = NULL;
  int tmp, ime=REG_IME;
  REG_IME = 0;

  
  IRQ_Enable(interrupt_type);

  if (-1 != (tmp=_L_ISR_LOCATIONS[interrupt_type])) {
    if (_G_ISR_HEAP_LAST_IDX==1) {
      ret = _G_ISR_TABLE[0].cb;
      _G_ISR_TABLE[0] = (IRQ_Callback_Entry_t){.priority = priority, .cb = isr, .flag = 1<<interrupt_type};
      return ret;
    }

    ret = _G_ISR_TABLE[tmp].cb;
    _G_ISR_TABLE[tmp] = _G_ISR_TABLE[--_G_ISR_HEAP_LAST_IDX];
    IRQ_Heap_Bubble_Down(tmp);
  }

  _L_ISR_LOCATIONS[interrupt_type] = IRQ_Heap_Insert(interrupt_type, isr, priority);
  REG_IME = ime;
  return ret;
}

IRQ_Callback_t IRQ_Remove(enum IRQ_Idx interrupt_type) {
  IRQ_Callback_t ret;
  int tmp, ime=REG_IME;
  REG_IME=0;
  if (-1 == (tmp=_L_ISR_LOCATIONS[interrupt_type])) {
    return NULL;
  }
  
  IRQ_Disable(interrupt_type);
  ret = _G_ISR_TABLE[tmp].cb;
  _L_ISR_LOCATIONS[interrupt_type] = -1;
  _G_ISR_TABLE[tmp] = _G_ISR_TABLE[--_G_ISR_HEAP_LAST_IDX];
  if (tmp == _G_ISR_HEAP_LAST_IDX) {
    REG_IME=ime;
    return ret;
  }

  IRQ_Heap_Bubble_Down(tmp);
  REG_IME = ime;
  return ret;
}

