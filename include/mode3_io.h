/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _MODE3_IO_H_
#define _MODE3_IO_H_


#include <sys/cdefs.h>
#ifdef __cplusplus
extern "C" {
#define restrict
#endif

int mode3_printf(int x, int y, unsigned short bg_clr, const char *restrict fmt, ...)
  __printflike(4,5);

void mode3_putchar(int c, int x, int y, unsigned short bg_color);



#ifdef __cplusplus
}
#udef restrict
#endif


#endif  /* _MODE3_IO_H_ */
