/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include "gba_types.h"
#include "queue.h"
#include <stddef.h>


void Queue_Enqueue(struct s_queue *queue, u16 data) {
  if (queue->len>=QUEUE_LIM) {
    return;
  }
  queue->buf[queue->tail++] = data;
  queue->tail &= QUEUE_MASK;
  ++queue->len;
}

u32 Queue_Dequeue(struct s_queue *queue) {
  if (queue->len<=0) {
    return 0xFFFF0000;
  }
  u16 ret;
  ret = queue->buf[queue->head++];
  queue->head &= QUEUE_MASK;
  
  if (--queue->len<=0) {
    Queue_Reset(queue);
  }
  return ret;
}

