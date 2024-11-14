#include "gba_mmap.h"
#include "gba_types.h"
#include "gba_util_macros.h"
#include "gba_funcs.h"

IRQ_Callback_Entry_t _G_ISR_TABLE[II_MAX] = {0};
int _G_ISR_HEAP_LAST_IDX = 0;
static bool _L_HAS_ISR[II_MAX]={0};

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




static int IRQ_Heap_Bubble_Down(int origin) {
  IRQ_Callback_Entry_t entry, *entp;
  int cidx;
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
    origin = cidx;
    
  }

  return origin;
  
}

static int IRQ_Heap_Bubble_Up(int origin) {
  IRQ_Callback_Entry_t ent, *entp;
  int pidx = (origin-1)/2;
  while (origin > 0 && _G_ISR_TABLE[origin].priority > _G_ISR_TABLE[pidx].priority) {
    ent = *(entp = _G_ISR_TABLE+origin);
    *entp = _G_ISR_TABLE[pidx];
    _G_ISR_TABLE[pidx] = ent;
    origin = pidx;
    pidx = (origin-1)/2;
  }
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

  if (_L_HAS_ISR[interrupt_type]) {
    if (_G_ISR_HEAP_LAST_IDX==1) {
      ret = _G_ISR_TABLE[0].cb;
      _G_ISR_TABLE[0] = (IRQ_Callback_Entry_t){.priority = priority, .cb = isr, .flag = 1<<interrupt_type};
      return ret;
    }
    tmp=-1;  // just so compiler will shut up. even tho tmp should never not be set after exiting for loop
    for (int i = 0; i < _G_ISR_HEAP_LAST_IDX; ++i) {
      if ((1<<interrupt_type)&_G_ISR_TABLE[i].flag) {
        tmp = i;
        break;
      }
    }

    ret = _G_ISR_TABLE[tmp].cb;
    _G_ISR_TABLE[tmp] = _G_ISR_TABLE[--_G_ISR_HEAP_LAST_IDX];
    IRQ_Heap_Bubble_Down(tmp);
  } else {
    _L_HAS_ISR[interrupt_type] = true;
  }
  IRQ_Heap_Insert(interrupt_type, isr, priority);
  REG_IME = ime;
  return ret;
}

IRQ_Callback_t IRQ_Remove(enum IRQ_Idx interrupt_type) {
  IRQ_Callback_t ret;
  int tmp, ime=REG_IME;
  REG_IME=0;
  if (!_L_HAS_ISR[interrupt_type]) {
    return NULL;
  }
  for (tmp=0; tmp < _G_ISR_HEAP_LAST_IDX; ++tmp) {
    if ((1<<interrupt_type)&_G_ISR_TABLE[tmp].flag) {
      break;
    }
  }
  
  IRQ_Disable(interrupt_type);
  ret = _G_ISR_TABLE[tmp].cb;
  
  _G_ISR_TABLE[tmp] = _G_ISR_TABLE[--_G_ISR_HEAP_LAST_IDX];
  IRQ_Heap_Bubble_Down(tmp);
  _L_HAS_ISR[interrupt_type] = false;

  REG_IME = ime;
  return ret;
}

