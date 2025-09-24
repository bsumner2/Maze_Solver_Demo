/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include "gba_funcs.h"
#include "gba_mmap.h"
#include "gba_types.h"
#include "mode3_io.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
static u8 *grid;
static int gwidth, gheight;


void draw_maze_cell(u8 *grid, const Coord_t *coord, int grid_width, int grid_height, u32 color);

void walk_traversal_draw_callback_register_params(u8 *param_grid, int param_gwidth, int param_gheight) {
  grid = param_grid;
  gwidth = param_gwidth;
  gheight = param_gheight;
} 

void walk_traversal_draw_cb(const void *userdata) {
  Coord_t c = *(Coord_t*)userdata;
  draw_maze_cell(grid, &c, gwidth, gheight, 0x7FFF);
//  vsync();
}

__attribute__ (( __noreturn__ )) void perrexit(const char *__restrict caller_name, int errno_value) {
  mode3_printf(0,0,0x1069, "[Error]:\x1b[0x10A5] Errno set by function called in, \x1b[0x2483]%s\x1b[0x10A5].\n"
      "Errno Value: \x1b[0x2483]%d\x1b[0x10A5]\tDetails: \x1b[0x1069]%s", caller_name, errno_value, strerror(errno_value));
  exit(EXIT_FAILURE);
}
