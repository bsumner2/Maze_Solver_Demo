/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include "bstree.h"
#include "graph.h"
#include "gba_def.h"
#include "gba_util_macros.h"
#include "input.h"
#include "linked_list.h"
#include "gba_types.h"
#include "gba_funcs.h"
#include "gba_mmap.h"
#include "mode3_io.h"

#ifdef _DEBUG_LOG_TO_SAVEFILE_
#include "sav_debug_log.h"
#endif

#include <stdlib.h>
#include <assert.h>

typedef enum e_direction {
  NONE_OR_START=0,
  LEFT=1,
  RIGHT=2,
  UP=4,
  DOWN=8,
  HORIZONTAL_MASK=3,
  VERTICAL_MASK=12
} __attribute__ ((packed)) Direction_e;

typedef enum e_maze_field {
  MF_LEFT_WALL=1,
  MF_RIGHT_WALL=2,
  MF_TOP_WALL=4,
  MF_BTM_WALL=8,
  MF_LR_WALLS=3,
  MF_TB_WALLS=12,
  MF_INITIALIZED=128
} __attribute__ ((packed)) MazeField_e;
#define COORD(_x,_y) (Coord_t){.x=_x, .y=_y}

extern int get_unclosed_block_ct(void);
  

typedef struct s_mvmt {
  Coord_t dest;
  Direction_e direction;
} Mvmt_t;

static const Coord_t EMPTY_COORD = COORD(0x80000000, 0x80000000);
static const Mvmt_t NO_MVMT = {EMPTY_COORD, NONE_OR_START};

LL_DECL(Mvmt_t, Mvmt);

#define Mvmt_LL_Dequeue(ll, dst) Mvmt_LL_Pop(ll, dst)
#define MSTACK_PEAK(ll) (ll->head->data)
#ifndef GRID_WIDTH
#define GRID_WIDTH 80
#endif  /* ! defined(GRID_WIDTH) */
#ifndef GRID_HEIGHT
#define GRID_HEIGHT 40
#endif  /* ! defined(GRID_HEIGHT) */

u8 GRID[GRID_HEIGHT][GRID_WIDTH];

void BinaryTree_Inorder(BinaryTree_t *tree, void (*traversal_callback)(const void*)) {
  BinaryTreeNode_t *node, **stack;
  bool *visited;
  int top=-1;
  stack = malloc(sizeof(void*)*BinaryTree_Element_Count(tree));
  if (!stack)
    return;
  visited = malloc(sizeof(bool)*BinaryTree_Element_Count(tree));
  stack[++top] = BinaryTree_Get_Root(tree);
  visited[top] = false;
  while (-1 < top) {
    node = stack[top];
    if (!node) {
      --top;
      continue;
    }
    if (visited[top]) {
      traversal_callback(node->data);
      stack[top] = node->r;
      visited[top] = false;
      continue;
    } 
    
    visited[top] = true;

    stack[++top] = node->l;
    visited[top] = false;
  }
  free(stack);
  free(visited);
}

void Mvmt_LL_Push(Mvmt_LL_t *ll, const Mvmt_t *data) {
  if (!ll || !data) return;
  Mvmt_t mvmt = *data;
  LL_NODE_PREPEND(Mvmt, ll, mvmt); 
}

void Mvmt_LL_Enqueue(Mvmt_LL_t *ll, const Mvmt_t *data) {
  if (!ll || !data) return;
  Mvmt_t mvmt = *data;
  LL_NODE_APPEND(Mvmt, ll, mvmt);
}

bool Mvmt_LL_Pop(Mvmt_LL_t *ll, Mvmt_t *dest) {
  if (!ll) return false;
  Mvmt_t local_dst;
  if (!(ll->nmemb))
      return false;
  LL_HEAD_RM(Mvmt, ll, local_dst);
  if (dest)
    *dest=local_dst;
  return true;
}
void Mvmt_LL_Close(Mvmt_LL_t *ll) {
  if (!ll) return;
  if (!(ll->nmemb))
    return;
  
  size_t ct = 0;
  for (Mvmt_LL_Node_t *node=ll->head, *nxt; node; node=nxt) {
    nxt=node->next;
    free(node);
    ++ct;
  }
  if  (ct!=ll->nmemb) {
    mode3_printf(0,0,0, "Discrepancy between number of nodes deallocated, %u, versus ll->nmemb starting value, %u", ct, ll->nmemb);
    exit(EXIT_FAILURE);
  }
  ll->nmemb=0UL;
}

STAT_INLN bool valid_grid_coord(Coord_t coord, int grid_width, int grid_height) {
  if (coord.x < 0 || coord.x >= grid_width) {
    return false;
  }
  return coord.y >= 0 && coord.y < grid_height;
}

/*

STAT_INLN bool Grid_Movement_Push(Mvmt_LL_t *mll, const Mvmt_t *movement, int grid_width, int grid_height) {

  if (!valid_grid_coord(movement->dest, grid_width, grid_height))
    return false;
  Mvmt_LL_Push(mll, movement);
  return true;
}

STAT_INLN bool Grid_Movement_Enqueue(Mvmt_LL_t *mll, const Mvmt_t *movement, int grid_width, int grid_height) {
  if (!valid_grid_coord(movement->dest, grid_width, grid_height))
    return false;
  Mvmt_LL_Enqueue(mll, movement);
  return true;
}

*/

STAT_INLN Mvmt_t Mvmt_LL_Peak(Mvmt_LL_t *mll) {
  if (!mll)
    return NO_MVMT;
  if (!(mll->nmemb))
    return NO_MVMT;
  return mll->head->data;
}

typedef struct s_walk {
  Coord_t start;
  Mvmt_LL_t path;
  BinaryTree_t *path_coord_set;
} Walk_t;

STAT_INLN int Coord_Cmp(Coord_t a, Coord_t b) {
  if (a.y!=b.y)
    return a.y-b.y;
  else
    return a.x-b.x;
}

STAT_INLN bool coords_eq(Coord_t a, Coord_t b) {
  return a.x == b.x && a.y==b.y;
}

static int coord_bst_cmpcb(const void *a, const void *b) {
  return Coord_Cmp(*(const Coord_t*)a, *(const Coord_t*)b);
}

void Walk_Init(Walk_t *walk, Coord_t start_coord) {
  walk->start = start_coord;
  walk->path = LL_INIT(Mvmt);
  walk->path_coord_set = BinaryTree_Create(malloc, free, NULL, NULL, coord_bst_cmpcb, sizeof(Coord_t), NULL);
#ifdef _DEBUG_LOG_TO_SAVEFILE_
  debug_log_printf("Starting new random walk at init position (%d, %d)\n", start_coord.x, start_coord.y);
#endif
  assert(BinaryTree_Insert(walk->path_coord_set, &start_coord));
}

void Walk_Close(Walk_t *walk) {
  walk->start = (Coord_t){.x=-1,.y=-1};
  Mvmt_LL_Close(&(walk->path));
  BinaryTree_Destroy(walk->path_coord_set); 
}

STAT_INLN Coord_t coords_sum(Coord_t a, Coord_t b) {
  return (Coord_t){.x = a.x+b.x, .y = a.y+b.y};
}

STAT_INLN Coord_t coords_diff(Coord_t minnuend, Coord_t subtrahend) {
  return (Coord_t){.x=minnuend.x - subtrahend.x, .y=minnuend.y - subtrahend.y};
}



Coord_t dir_to_coord(Direction_e dir) {
  switch (dir) {
  case NONE_OR_START:
    return COORD(0, 0);
  case LEFT:
    return COORD(-1, 0);
  case RIGHT:
    return COORD(1, 0);
  case UP:
    return COORD(0, -1);
  case DOWN:
    return COORD(0, 1);
  default:
    return EMPTY_COORD;
  }
}


#ifdef _DEBUG_LOG_TO_SAVEFILE_
void debug_print__coordtree_traversal_cb(const void *data) {
  Coord_t coord = *(const Coord_t*)data;
  debug_log_printf("Tree Entry: (%d, %d)\n", coord.x, coord.y);
}
#endif  /* DEBUG LOGS TO .SAV FILE */
#define _DRAW_WALK_  // TODO: DELETE THIS MACRO DEFINE
bool Walk_Advance(Walk_t *walk, Direction_e dir, int grid_width, int grid_height) {
#ifdef _DRAW_WALK_
  BMP_Rect_t r = {
    .x = 0, .y = 0,
    .width = SCREEN_WIDTH/grid_width, .height = SCREEN_HEIGHT/grid_height, 
    .color = 0
  };
#endif
  Mvmt_t head, newhead={EMPTY_COORD, NONE_OR_START};
  Mvmt_LL_t *path = &(walk->path);
  if (!walk) return false;
  if (path->nmemb==0) {
    head = (Mvmt_t){.dest=walk->start, .direction=NONE_OR_START};
    Direction_e dirmask=head.direction;
    if (dirmask&HORIZONTAL_MASK) {
      dirmask = HORIZONTAL_MASK;
    } else {
      dirmask = VERTICAL_MASK;
    }

    if (dir&dirmask) {
      if (head.direction!=dir) {
        // if this is the case, then direction is opposite to last movement, 
        // thus leading back to coord of previous position (aka second-to-top
        // of stack)
        return false;
      }
    }
  } else {
    head = Mvmt_LL_Peak(path);
  }

  Coord_t dst = coords_sum(head.dest, dir_to_coord(dir));
  if (!valid_grid_coord(dst, grid_width, grid_height))
    return false;
  if (BinaryTree_Contains(walk->path_coord_set, &dst)) {
#ifdef _DEBUG_LOG_TO_SAVEFILE_
  debug_log_printf("Tree contains: (%d, %d)\n", dst.x, dst.y);
  debug_log_printf("Tree element ct: %u\n", BinaryTree_Element_Count(walk->path_coord_set));
  debug_log_printf("Path LL element ct: %u\n", path->nmemb);
    BinaryTree_Inorder(walk->path_coord_set, debug_print__coordtree_traversal_cb);
    {
      Coord_t c;
      int idx = 0;
      LL_FOREACH(LL_NODE_VAR_INITIALIZER(Mvmt, cur), cur, path) {
        c = cur->data.dest;
        debug_log_printf("path[%d] = (%d, %d)\n", idx++, c.x, c.y);
      }
    }
#endif  /* DEBUG LOGS TO .SAV FILE */
    while (path->nmemb!=0UL) {
      head = Mvmt_LL_Peak(path);
      if (coords_eq(head.dest, dst))
        break;
      Mvmt_LL_Pop(path, &head);
      assert(BinaryTree_Remove(walk->path_coord_set, &head.dest));
#ifdef _DRAW_WALK_
      r.x = head.dest.x*r.width;
      r.y = head.dest.y*r.height;
      r.color = 0;
      mode3_draw_rect(&r);
#endif
    }
    assert((path->nmemb!=0UL) || 
        (BinaryTree_Element_Count(walk->path_coord_set)==1 &&
          coords_eq(walk->start, dst) &&
          BinaryTree_Contains(walk->path_coord_set, &dst)));
    return false;
  }
  BinaryTree_Insert(walk->path_coord_set, &dst);
  newhead.dest = dst;
  newhead.direction = dir;
  Mvmt_LL_Push(path, &newhead);
#ifdef _DRAW_WALK_
  r.x = newhead.dest.x*r.width;
  r.y = newhead.dest.y*r.height;
  r.color = 0x7A08;
  mode3_draw_rect(&r);
#endif
  return true;
}

STAT_INLN Coord_t randcoord(int grid_width, int grid_height) {
  return COORD(rand()%grid_width, rand()%grid_height);
}

STAT_INLN Direction_e randdir(void) {
  u32 r = rand();
  if (r&2) {
    return r&1 ? RIGHT : LEFT;
  } else if (r&1) {
    return DOWN;
  } else {
    return UP;
  }
}

#define CIDX(coord, gwidth) (coord.x + coord.y*gwidth)

void Walk(Walk_t *walk, const u8 *grid, int grid_width, int grid_height) {
  if (!walk || !grid) return;
  if (BinaryTree_Element_Count(walk->path_coord_set)!=0) {
    Walk_Close(walk);
  }

  Coord_t start;
  do {
    start = randcoord(grid_width, grid_height);
  } while ((grid[CIDX(start, grid_width)] & MF_INITIALIZED));
#ifdef _DRAW_WALK_
  BMP_Rect_t r = {.width = SCREEN_WIDTH/grid_width, .height=SCREEN_HEIGHT/grid_height, .x=0,.y=0,.color = 0x7FFF};
  r.x = start.x*r.width;
  r.y = start.y*r.height;
  mode3_draw_rect(&r);
#endif

  Walk_Init(walk, start);
  Mvmt_t head;
  bool advanced;
  for (;;) {
    do {
      advanced = Walk_Advance(walk, randdir(), grid_width, grid_height);
//      vsync();
    } while (!advanced);
    if (walk->path.nmemb==0)
      continue;
    head = Mvmt_LL_Peak(&(walk->path));
    if (grid[CIDX(head.dest, grid_width)] & MF_INITIALIZED)
      return;
  }
}

void mode3_draw_line(Coord_t start, u32 color, int length, bool vertical) {
  if (!valid_grid_coord(start, SCREEN_WIDTH, SCREEN_HEIGHT)) return;
  if (length <= 0) return;

  u16 *vr = VRAM_BUF + start.x + start.y*SCREEN_WIDTH;
  if (vertical) {
    if (start.y + length > SCREEN_HEIGHT)
      return;
    do {
      *vr = color;
      vr+= SCREEN_WIDTH;
    } while (--length);
  } else {
    if (start.x + length > SCREEN_WIDTH)
      return;
    do {
      *vr++ = color;
    } while (--length);
  }
}

void draw_maze_cell(u8 *grid, const Coord_t *coord, int grid_width, int grid_height, u32 color) {
  Coord_t c = *coord;
  if (!valid_grid_coord(c, grid_width, grid_height))
    return;
  BMP_Rect_t r = {.x=0,.y=0,.width=SCREEN_WIDTH/grid_width, .height = SCREEN_HEIGHT/grid_height, .color = color};
  r.x = c.x*r.width;
  r.y = c.y*r.height;
  u8 cell = grid[CIDX(c, grid_width)];
  if (!(cell&MF_INITIALIZED)) {
    r.color = 0;
    mode3_draw_rect(&r);
    return;
  }
  Coord_t rc = COORD(c.x*r.width, c.y*r.height);
  mode3_draw_rect(&r);
  if (cell&MF_LEFT_WALL) {
    mode3_draw_line(rc, 0x10A5, r.height, true);
  }
  if (cell&MF_RIGHT_WALL) {
    int orig = rc.x;
    rc.x += r.width-1;
    mode3_draw_line(rc, 0x10A5, r.height, true);
    rc.x = orig;
  }

  if (cell&MF_TOP_WALL) {
    mode3_draw_line(rc, 0x10A5, r.width, false);
  }

  if (cell&MF_BTM_WALL) {
    rc.y += r.height-1;
    mode3_draw_line(rc, 0x10A5, r.width, false);
  }
}
#define FORCE_ASSERTION_FAILURE false

#define IDX(x,y,gw) (x+y*gw)



void Incorporate_Walk(u8 *grid, Walk_t *walk, int grid_width) {
  Mvmt_LL_t *path = &(walk->path);
  Mvmt_t curmove;
  assert(Mvmt_LL_Pop(path, &curmove));
  assert(grid[CIDX(curmove.dest, grid_width)]&MF_INITIALIZED);
  switch (curmove.direction) {
  case NONE_OR_START:
    assert(FORCE_ASSERTION_FAILURE);
    break;
  case LEFT:
    grid[CIDX(curmove.dest, grid_width)] ^= MF_RIGHT_WALL;
    ++curmove.dest.x;
    grid[CIDX(curmove.dest, grid_width)] ^= MF_LEFT_WALL;
    break;
  case RIGHT:
    grid[CIDX(curmove.dest, grid_width)] ^= MF_LEFT_WALL;
    --curmove.dest.x;
    grid[CIDX(curmove.dest, grid_width)] ^= MF_RIGHT_WALL;
    break;
  case UP:
    grid[CIDX(curmove.dest, grid_width)] ^= MF_BTM_WALL;
    ++curmove.dest.y;
    grid[CIDX(curmove.dest, grid_width)] ^= MF_TOP_WALL;
    break;
  case DOWN:
    grid[CIDX(curmove.dest, grid_width)] ^= MF_TOP_WALL;
    --curmove.dest.y;
    grid[CIDX(curmove.dest, grid_width)] ^= MF_BTM_WALL;
    break;
  case HORIZONTAL_MASK:
  case VERTICAL_MASK:
    assert(curmove.direction!=HORIZONTAL_MASK && curmove.direction!=VERTICAL_MASK);
    break;
  }

  while (path->nmemb) {
    assert(Mvmt_LL_Pop(path, &curmove));
    grid[CIDX(curmove.dest, grid_width)] |= MF_INITIALIZED;
    switch (curmove.direction) {
    case NONE_OR_START:
      assert(curmove.direction!=NONE_OR_START);
      break;
    case LEFT:
      grid[CIDX(curmove.dest, grid_width)] ^= MF_RIGHT_WALL;
      ++curmove.dest.x;
      grid[CIDX(curmove.dest, grid_width)] ^= MF_LEFT_WALL;
      break;
    case RIGHT:
      grid[CIDX(curmove.dest, grid_width)] ^= MF_LEFT_WALL;
      --curmove.dest.x;
      grid[CIDX(curmove.dest, grid_width)] ^= MF_RIGHT_WALL;
      break;
    case UP:
      grid[CIDX(curmove.dest, grid_width)] ^= MF_BTM_WALL;
      ++curmove.dest.y;
      grid[CIDX(curmove.dest, grid_width)] ^= MF_TOP_WALL;
      break;
    case DOWN:
      grid[CIDX(curmove.dest, grid_width)] ^= MF_TOP_WALL;
      --curmove.dest.y;
      grid[CIDX(curmove.dest, grid_width)] ^= MF_BTM_WALL;
      break;
    case HORIZONTAL_MASK:
    case VERTICAL_MASK:
      assert(FORCE_ASSERTION_FAILURE);
      break;
    }
  }

}

void walk_traversal_draw_callback_register_params(u8 *param_grid, int param_gwidth, int param_gheight);
void walk_traversal_draw_cb(const void *userdata);

void Wilsons_Algo(u8 *grid, int grid_width, int grid_height) {
  const u32 GRID_CELL_TOTAL = grid_width*grid_height;
  u32 initialized_ct=0UL;
  walk_traversal_draw_callback_register_params(grid, grid_width, grid_height);
  fast_memset32(grid, 0x0F0F0F0F, grid_width*grid_height/4);
  {
    int remainder;
    if ((remainder=(grid_width*grid_height)&3)) {

      u8 *rem = &grid[(grid_width*grid_height)&(~3)];
      while (remainder--) {
        *rem++ = 0;
      }
    }
  }

  Coord_t init = randcoord(grid_width, grid_height);
  grid[CIDX(init, grid_width)] |= MF_INITIALIZED;
  assert(++initialized_ct==1);
#ifdef _DRAW_WALK_
  draw_maze_cell(grid, &init, grid_width, grid_height, 0x7FFF);
#endif
  Walk_t walk={0};
  do {
    Walk(&walk, grid, grid_width, grid_height);
    initialized_ct += walk.path.nmemb;
    Incorporate_Walk(grid, &walk, grid_width);
    BinaryTree_Inorder(walk.path_coord_set, walk_traversal_draw_cb);
    
 
    grid[CIDX(walk.start, grid_width)] |= MF_INITIALIZED;
    draw_maze_cell(grid, &walk.start, grid_width, grid_height, 0x7FFF);
/*    for (Coord_t c = COORD(0,0); c.y < grid_height; ++c.y) {
      for (c.x = 0; c.x < grid_width; ++c.x) {
        draw_maze_cell(grid, &c, grid_width, grid_height);
      }
    }
*/
    Walk_Close(&walk);
  } while (initialized_ct < GRID_CELL_TOTAL);
}


int vicmp(const void *a, const void *b) {
  return (int)(((uintptr_t)a)-((uintptr_t)b));
}



typedef Coord_t Vec2;

STAT_INLN bool Valid_Mvmt(u8 *grid, const Coord_t *origin, const Mvmt_t *move, 
    const Vec2 *grid_dims) {
  // for sake of inline brevity, skipping params' validity check
  Coord_t coord = coords_sum(*origin, move->dest);
  if (!valid_grid_coord(coord, grid_dims->x, grid_dims->y))
    return false;
  coord = *origin;
  switch (move->direction) {
  case NONE_OR_START:
    return true;
  case VERTICAL_MASK:
    return false;
  case HORIZONTAL_MASK:
    return false;
  default:
    return !(grid[CIDX(coord, grid_dims->x)]&(move->direction));
  }

}


Graph_t *Graph_Maze(u8 *grid, const Coord_t *start, const Coord_t *end, const Vec2 *grid_dims) {
  if (!grid || !start || !end || !grid_dims)
    return NULL;
  Coord_t startpt = *start, endpt = *end, curr_coord, move_coord;
  Graph_t *ret =  NULL;
  int grid_width = grid_dims->x, grid_height = grid_dims->y;
  ret = Graph_Init(NULL, NULL, coord_bst_cmpcb, sizeof(Coord_t));
  
  assert(Graph_Add_Vertex(ret, &startpt));

  draw_maze_cell(grid, &startpt, grid_width, grid_height, 0x7A08);
  assert(Graph_Add_Vertex(ret, &endpt));

  draw_maze_cell(grid, &endpt, grid_width, grid_height, 0x7A08);
  Mvmt_LL_t valid_dirs = LL_INIT(Mvmt);
  Mvmt_t tmp;
  GraphNode_t *curr_vert;
  size_t curr_idx = 0;
  u8 maze_cell_fields;

  do {
    curr_vert = &(ret->vertices[curr_idx]);
    curr_coord = *(Coord_t*)(curr_vert->data);
    maze_cell_fields = grid[CIDX(curr_coord, grid_width)];
    for (int i=1; i&15; i<<=1) {
      tmp = (Mvmt_t) {.dest = dir_to_coord(i), .direction = i};
      if (!valid_grid_coord(coords_sum(tmp.dest, curr_coord),
            grid_width, grid_height))
        continue;
      if (maze_cell_fields&i)
        continue;
      Mvmt_LL_Push(&valid_dirs, &tmp);
    }
    while (valid_dirs.nmemb) {
      Mvmt_LL_Pop(&valid_dirs, &tmp);
      move_coord = curr_coord;
      do {
        move_coord.x += tmp.dest.x;
        move_coord.y += tmp.dest.y;
        draw_maze_cell(grid, &move_coord, grid_width, grid_height, 0x6739);
        maze_cell_fields = grid[CIDX(move_coord, grid_width)];
        if (tmp.direction&HORIZONTAL_MASK) {
          maze_cell_fields |= MF_LR_WALLS;
        } else {
          maze_cell_fields |= MF_TB_WALLS;
        }
        
        maze_cell_fields = ~maze_cell_fields;
        maze_cell_fields &= 15;
        if (maze_cell_fields) {
          if (Graph_Add_Vertex(ret, &move_coord)) {
            Coord_t diff = coords_diff(curr_coord, move_coord);
            int weight;
            if (tmp.direction&HORIZONTAL_MASK) {
              assert(diff.y==0);
              if (tmp.direction == RIGHT) {
                assert(0 > diff.x);
                diff.x = -diff.x;
              }
              weight = diff.x;
            } else {
              assert(diff.x==0);
              if (tmp.direction == DOWN) {
                assert(0 > diff.y);
                diff.y = -diff.y;
              }
              
              weight = diff.y;
            }
            assert(0 < weight);
            assert(coords_eq(*(Coord_t*)ret->vertices[ret->vertex_ct-1].data, move_coord));
            assert(Graph_Add_TwoWay_Edge(ret, curr_idx, ret->vertex_ct-1, weight));

          } else {
            GraphNode_t *mv_vert;
            bool already_linked=false;
            assert((mv_vert = Graph_Get_Vertex(ret, &move_coord))!=NULL);
            assert(coords_eq(*(Coord_t*)(mv_vert->data), move_coord));
            GraphEdge_LL_t *adjs = &(curr_vert->adj_list);
            LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
              if (node->data.dst_idx == mv_vert->idx) {
                already_linked = true;
                break;
              }
            }
            if (already_linked) {
              adjs = &(mv_vert->adj_list);
              already_linked = false;
              LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
                if ((size_t)(node->data.dst_idx)==curr_idx) {
                  already_linked = true;
                  break;
                }
              }
              assert(already_linked);
            } else {
              Coord_t diff = coords_diff(curr_coord, move_coord);
              int weight;
              if (tmp.direction&HORIZONTAL_MASK) {
                assert(diff.y==0);
                if (tmp.direction == RIGHT) {
                  assert(0 > diff.x);
                  diff.x = -diff.x;
                }
                weight = diff.x;
              } else {
                assert(diff.x==0);
                if (tmp.direction == DOWN) {
                  assert(0 > diff.y);
                  diff.y = -diff.y;
                }
                
                weight = diff.y;
              }
              assert(0 < weight);
              assert(Graph_Add_TwoWay_Edge(ret, curr_idx, mv_vert->idx, weight));
            }
            
          }
          draw_maze_cell(grid, &move_coord, grid_width, grid_height, 0x7A08);
          if (ret->vertices+curr_idx!= curr_vert)
            curr_vert = ret->vertices+curr_idx;
          break;
        }
      } while (Valid_Mvmt(grid, &move_coord, &tmp, grid_dims));
    }
  } while (++curr_idx < ret->vertex_ct);

  

  return ret;
}

typedef struct s_dve {
  u32 vertex_index;
  u32 distance;
} DijkstraVertent_t;

static int dve_cmp(const void *a, const void *b) {
  const DijkstraVertent_t *da = a, *db = b;
  if (da->distance < db->distance)
    return -1;
  else if (da->distance > db->distance)
    return 1;
  else
    return da->vertex_index - db->vertex_index;
}

STAT_INLN Coord_t get_movement_vector(Coord_t src, Coord_t dst) {
  src = coords_diff(src, dst);
  assert((src.x==0) ^ (src.y == 0));
  if (src.x) {
    if (src.x > 0)
      src.x = 1;
    else
      src.x = -1;
    return src;
  }
  if (src.y) {
    if (src.y > 0)
      src.y = 1;
    else
      src.y = -1;
    return src;
  }
  return COORD(0,0);
}

static void draw_maze_path(u8 *grid, const Coord_t *src, const Coord_t *dst, const Coord_t *grid_dims, u32 color) {
  Coord_t c=*src, dstc=*dst, mv = get_movement_vector(dstc, c);
  int gw = grid_dims->x, gh = grid_dims->y;
  assert(mv.x != mv.y);
  c = coords_sum(c, mv);
  while (!coords_eq(c, dstc)) {
    draw_maze_cell(grid, &c, gw, gh, color);
    vsync();
    c = coords_sum(c, mv);
  }
}
GraphNode_t **Dijkstras(Graph_t *graph, u32 src, u32 dst) {
  GraphNode_t **prevs;
  u32 *dist;
  if (!graph)
    return NULL;
  if (src < 0 || src >= graph->vertex_ct)
    return NULL;
  if (dst < 0 || dst >= graph->vertex_ct)
    return NULL;

  BinaryTree_t *unvisited = BinaryTree_Create(malloc, free, NULL, NULL, dve_cmp, sizeof(DijkstraVertent_t), NULL);
  assert(NULL!=unvisited);

  {
    DijkstraVertent_t tmp = {.vertex_index=0U, .distance = 0xFFFFFFFFUL};
    const size_t LIM = graph->vertex_ct;

    u32 *tmpdist;
    dist = tmpdist = (u32*)malloc(sizeof(u32)*LIM);

    prevs = (GraphNode_t**)calloc(LIM, sizeof(void*));

    assert(dist != NULL && prevs != NULL);

    for (tmp.vertex_index=0UL; tmp.vertex_index < LIM; ++tmp.vertex_index) {
      BinaryTree_Insert(unvisited, &tmp);
      *tmpdist++ = 0xFFFFFFFFU;
    }
    
    tmp.vertex_index = src;
    assert(BinaryTree_Remove(unvisited, &tmp)==1);
  
    tmp.distance = dist[src] = 0UL;

    assert(BinaryTree_Insert(unvisited, &tmp)==1);
  }
  
  DijkstraVertent_t tree_query={0}, *curvertent;
  Coord_t dims = COORD(GRID_WIDTH, GRID_HEIGHT);
  GraphNode_t *curvert;
  GraphEdge_LL_t *curvert_adjlist;
  u32 nextdist, curdist, altdist, next_idx;
  while (BinaryTree_Element_Count(unvisited)) {
    assert((curvertent= BinaryTree_Remove_Minimum(unvisited))!=NULL);
    assert(curvertent->distance!=0xFFFFFFFFUL);
    if (curvertent->vertex_index == dst) {
      assert(prevs[dst]!=NULL && dist[dst]!=0xFFFFFFFFUL);
      free((void*)curvertent);
      break;
    }
    curvert = graph->vertices + (curvertent->vertex_index);
    draw_maze_cell((u8*)GRID, curvert->data, GRID_WIDTH, GRID_HEIGHT, 0x7A08);
    curvert_adjlist = &(curvert->adj_list);
    curdist = curvertent->distance;
    LL_FOREACH (LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, curvert_adjlist) {
      tree_query.vertex_index = next_idx = node->data.dst_idx;
      tree_query.distance = nextdist = dist[next_idx];
      bool node_unvisited = BinaryTree_Contains(unvisited, &tree_query);
      if (!node_unvisited) {
        continue;
      }

      altdist = (unsigned)node->data.weight + curdist;
      if (altdist >= nextdist) {
        continue;
      }
      draw_maze_path((u8*)GRID, curvert->data, graph->vertices[next_idx].data, &dims, 0x6739);
      tree_query.distance = nextdist;
      tree_query.vertex_index = next_idx;
      assert(BinaryTree_Remove(unvisited, &tree_query)==1);
      tree_query.distance = dist[next_idx] = altdist;
      assert(BinaryTree_Insert(unvisited, &tree_query)==1);
      prevs[next_idx] = curvert;

    }
    free((void*)curvertent);
  }
  free((void*)dist);
  BinaryTree_Destroy(unvisited);
  return prevs;

}



static void draw_maze(u8 *grid, int grid_width, int grid_height) {
  Coord_t c;
  for (c.y = 0; c.y < grid_height; ++c.y) {
    for (c.x= 0; c.x < grid_width; ++c.x) {
      draw_maze_cell(grid, &c, grid_width, grid_height, 0x7FFF);
    }
  }
}

int main(void) {
  IRQ_Init(NULL);
  IRQ_Add(II_VBLANK, NULL, II_MAX);
  REG_DISPLAY_CNT=0x0403;
#ifdef RNG_SEED
  srand(RNG_SEED);
#elif 1
  // Cool maze seeds :)
//  srand(0x27D9440DUL);
//  srand(0x3CB76F4DUL);
//  srand(0x6FAB21B7UL);
//  srand(0x3D1E32C6UL);
//  srand(0x801BC0C1UL);
//  srand(0x0A99B79BUL);
//  srand(0x043EC82CUL);
//  srand(0xAA41D7F7UL);
//  srand(0x3777C479UL);
//  srand(0x95867A5FUL);
//  srand(0x27D9440EUL);
//  srand(0xE573C496UL);
//  srand(0x7612FB02UL);
//  srand(0xA739BD0AUL);
  srand(0x4DA23792UL);
#endif
#ifdef _DEBUG_LOG_TO_SAVEFILE_
  debug_log_initialize();
#endif  /* DEBUG LOGS TO .SAV FILE */
  Wilsons_Algo((u8*)GRID, GRID_WIDTH, GRID_HEIGHT);
  Coord_t start=COORD(0,0), end=COORD(GRID_WIDTH-1, GRID_HEIGHT-1), dims=COORD(GRID_WIDTH,GRID_HEIGHT), *c;
  Graph_t *maze_graph = Graph_Maze((u8*)GRID, &start, &end, &dims);
  assert(maze_graph!=NULL);

  do vsync(); while (Poll_Keys(), !K_STROKE(START));
  draw_maze((u8*)GRID, GRID_WIDTH, GRID_HEIGHT);
  
  const size_t sz = maze_graph->vertex_ct;
  GraphNode_t *vertices = maze_graph->vertices;
  GraphEdge_LL_t *adjs;

  
  for (size_t i = 0; i < sz; ++i) {

    c = vertices[i].data;
    draw_maze_cell((u8*)GRID, c, GRID_WIDTH, GRID_HEIGHT, 0x7A08);
    adjs = Graph_Get_Vertex_Adjacents(maze_graph, i);
    assert(adjs!=NULL);
    LL_FOREACH (LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
      draw_maze_path((u8*)GRID, c, vertices[node->data.dst_idx].data, &dims, 0x6739);
    }
    vsync();
  }
  
  do vsync(); while (Poll_Keys(), !K_STROKE(START));
  draw_maze((u8*)GRID, GRID_WIDTH, GRID_HEIGHT);

  GraphNode_t **prevs = Dijkstras(maze_graph, 0, 1), **stack = malloc(sizeof(void*)*(maze_graph->vertex_ct)), *cur, *nxt;
  assert(prevs!=NULL && stack!=NULL);
  do vsync(); while (Poll_Keys(), !K_STROKE(START));
  
  draw_maze((u8*)GRID, GRID_WIDTH, GRID_HEIGHT);
  vsync();
  int top = -1;
  int idx = 1;
  draw_maze_cell((u8*)GRID, vertices[idx].data, GRID_WIDTH, GRID_HEIGHT, 0x6739);
  while ((cur=prevs[idx])) {
    vsync();
    draw_maze_cell((u8*)GRID, cur->data, GRID_WIDTH, GRID_HEIGHT, 0x6739);
    stack[++top] = vertices+idx;
    idx = cur - vertices;
  }
  free(prevs);
  do vsync(); while (Poll_Keys(), !K_STROKE(START));
  assert(idx == 0);
  cur = vertices;
  while (-1 < top) {
    draw_maze_cell(((u8*)GRID), cur->data, GRID_WIDTH, GRID_HEIGHT, 0x7A08);
    vsync();
    nxt = stack[top--];
    draw_maze_path((u8*)GRID, cur->data, nxt->data, &dims, 0x7A08);
    cur = nxt;
  }

  // TODO: Fix Graph_Maze so that vertices that spawned from dst traversal have connecting edges to those from src
  
  free(stack);
  Graph_Close(maze_graph);

    
  while (1);
}

