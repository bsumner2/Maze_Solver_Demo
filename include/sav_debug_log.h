#ifndef _SAV_DBG_LOG_H_
#define _SAV_DBG_LOG_H_

#include "gba_util_macros.h"
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

void debug_log_initialize(void);

_Bool debug_log_printf(const char *restrict fmt, ...) 
  __printflike(1,2);
#ifdef __cplusplus
}
#endif

#endif  /* _SAV_DBG_LOG_H_ */
