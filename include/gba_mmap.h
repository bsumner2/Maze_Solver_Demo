/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _GBA_MMAP_H_
#define _GBA_MMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define REG_BASE          0x04000000
#define MEM_PAL           0x05000000
#define MEM_VRAM          0x06000000
#define MEM_OAM           0x07000000
#define MEM_SRAM          0x0E000000


#define REG_DISPLAY_CNT (*((volatile u32*) (REG_BASE)))
#define REG_DISPLAY_STAT (*((volatile u16*) (REG_BASE+0x0004)))
#define REG_DISPLAY_VCOUNT (*((volatile u16*) (REG_BASE+0x0006)))

#define REG_SIO_CNT (*((volatile u16*) (REG_BASE+0x0128)))
#define REG_SIO_MULTI_SEND (*((volatile u16*) (REG_BASE+0x012A)))
#define REG_SIO_RCNT (*((volatile u16*) (REG_BASE+0x0134)))


#define REG_IME (*((volatile u16*) (REG_BASE + 0x0208)))
#define REG_IE  (*((volatile u16*) (REG_BASE + 0x0200)))
#define REG_IF  (*((volatile u16*) (REG_BASE + 0x0202)))
#define REG_IFBIOS (*((volatile u16*) (0x03007FF8)))

#define REG_ISR_MAIN (*((IRQ_Callback_t*) (0x03007FFC)))

#define REG_KEY_STAT (*((volatile u16*) (REG_BASE+0x0130)))
#define REG_KEY_CNT (*((volatile u16*) (REG_BASE+0x0132)))

#define VRAM_BUF ((u16*) MEM_VRAM)

#define TILE_MEM ((Charblock*) MEM_VRAM)
#define TILE8_MEM ((Charblock8*) MEM_VRAM)

#define PAL_BG_MEM ((u16*) MEM_PAL)
#define PAL_OBJ_MEM ((u16*) (MEM_PAL + 0x0200))

#define OAM_MEM ((Obj_Attrs_t*) MEM_OAM)
#define AFFINE_MEM ((Obj_Affine_t*) MEM_OAM)

#define SRAM_BUF ((u8*) (MEM_SRAM))

#define SIO_MULTI_DATA(slot) (*((volatile u16*) (REG_BASE + ((slot&3)<<1) + 0x0120)))

#define REG_TIMER_CNT(timer_no) (((volatile u16*) (REG_BASE+ ((timer_no&3)<<2) + 0x102)))
#define REG_TIMER_DATA(timer_no) (((volatile u16*) (REG_BASE+ ((timer_no&3)<<2) + 0x100)))

#define REG_TIMER(timer_no) (*((volatile Timer_t*) (REG_BASE+((timer_no&3)<<2) + 0x100)))



#ifdef __cplusplus
}
#endif

#endif  /* _GBA_MMAP_H_ */
