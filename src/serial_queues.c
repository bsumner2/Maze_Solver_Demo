#include "gba_mmap.h"
#include "gba_types.h"
#include "queue.h"

static Queue_t i_queue, o_queue, *q0, *q1;

Queue_t *SIO_Get_Queue(bool in_queue) {
  return in_queue?&i_queue:&o_queue;
}

void SIO_SetQueues(void) {
  if (((SIO_Cnt_t){.raw=REG_SIO_CNT}).multi.SI) {
    q0 = &i_queue, q1 = &o_queue;
  } else {
    q0=&o_queue, q1=&i_queue;
  }
  Queue_Reset(q0);
  Queue_Reset(q1);
}

void SIO_Queue_Serial_CB(void) {
  static u16 data[2]={0};
  *((u32*)data) = *((volatile u32*) (&SIO_MULTI_DATA(0)));

  if (data[0] && data[0]!=0xFFFF)
    Queue_Enqueue(q0, data[0]);
  if (data[1] && data[1]!=0xFFFF)
    Queue_Enqueue(q1, data[1]);
}
