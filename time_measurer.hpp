#ifndef _TIME_MEASURER_
#define _TIME_MEASURER_

#ifdef __cplusplus
extern "C"{
#endif

#include <time.h>
#include <stdio.h>
#include <inttypes.h>

struct timespec begin, end;
uint64_t dura;

inline
void time_start(){
    clock_gettime(CLOCK_REALTIME, &begin);
}

inline
void time_stop(){
    clock_gettime(CLOCK_REALTIME, &end);
}

inline
void time_report(){
    dura = ((int64_t)end.tv_sec*1000000000 + end.tv_nsec) - ((int64_t)begin.tv_sec*1000000000 + begin.tv_nsec);//ns
    printf("Total time elapsed : %lu ns\n", dura);
}

#ifdef __cplusplus
}
#endif


#endif
