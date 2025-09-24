/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "gba_funcs.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "gba_types.h"
typedef struct s_queue Queue_t;


#define QUEUE_LIM 32
#define QUEUE_MASK 31

struct s_queue {
  u16 buf[QUEUE_LIM];
  u16 head, tail, len;
};


static inline void Queue_Reset(Queue_t *queue) {
  fast_memset32(queue->buf, 0, sizeof(queue->buf)/4);
  *((u32*) (&queue->head)) = 0;  // take care of head and tail simultaneously
  queue->len = 0;
}

static inline u32 Queue_Peak(Queue_t *queue) {
  if (queue->len <= 0)
    return 0xFFFF0000;
  return queue->buf[queue->head];
}

void Queue_Enqueue(Queue_t *queue, u16 data);
u32 Queue_Dequeue(Queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif  /* _QUEUE_H_ */
