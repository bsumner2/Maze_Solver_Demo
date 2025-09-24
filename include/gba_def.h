/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _GBA_DEF_H_
#define _GBA_DEF_H_

#include "gba_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_HEIGHT 160
#define SCREEN_WIDTH  240

#define KEY_A     1
#define KEY_B     2
#define KEY_SEL   4
#define KEY_START 8
#define KEY_RIGHT 16
#define KEY_LEFT  32
#define KEY_UP    64
#define KEY_DOWN  128
#define KEY_R     256
#define KEY_L     512

#define OBJ_MODE_REGULAR 0
#define OBJ_MODE_AFFINE 1
#define OBJ_MODE_HIDE 2
#define OBJ_MODE_AFFINE_X2RENDERING 3

#define IRQ_VBL 1
#define IRQ_HBL 2
#define IRQ_VCT 4
#define IRQ_TM  8
#define IRQ_COM 16
#define IRQ_DMA 32
#define IRQ_KEY 64
#define IRQ_PAK 128

#define TM_FREQ_1HZ     0
#define TM_FREQ_64HZ    1
#define TM_FREQ_256HZ   2
#define TM_FREQ_1024HZ  3

#ifdef __cplusplus
}
#endif









#endif  /* _GBA_DEF_H_ */
