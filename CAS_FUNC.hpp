#ifndef __CAS_FUNC__
#define __CAS_FUNC__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

    bool DO_CAS(volatile int * ptr, int oldval, int newval);

#ifdef __cplusplus
}
#endif
#endif
