#ifndef _SERIAL_QUEUES_H_
#define _SERIAL_QUEUES_H_

#include "queue.h"
#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

void SIO_SetQueues(void);
void SIO_Queue_Serial_CB(void);
Queue_t *SIO_Get_Queue(bool in_queue);

#define SIO_Get_InQueue() SIO_Get_Queue(true)
#define SIO_Get_OutQueue() SIO_Get_Queue(false)

#ifdef __cplusplus
}
#endif


#endif  /* _SERIAL_QUEUES_H_ */
